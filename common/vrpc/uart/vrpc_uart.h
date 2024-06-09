#pragma once

#include <array>
#include <memory>
#include <ranges>
#include <span>

#include <hal/system.h>
#include <hal/uart.h>

#include <constexpr_tools/helpers.h>
#include <constexpr_tools/math.h>

#include "vrpc_uart_decoder.h"
#include "vrpc_uart_encode.h"

namespace vrpc::uart {

enum class HandleState {
  Handled,
  HandlingAsync,
  ErrUnknownCommand,
  ErrMalformedPayload,
  ErrEncodeFailure,
};

struct HandleResult {
  HandleState                state;
  std::span<const std::byte> response_payload;
};

template <typename Impl>
concept ServiceImpl = requires(Impl& impl) {
  { Impl::ServiceId } -> std::convertible_to<uint32_t>;

  requires ct::IsConstantExpression<Impl::MinBufferSize()>();
  { Impl::MinBufferSize() } -> std::convertible_to<std::size_t>;

  {
    impl.HandleCommand(
        std::declval<uint32_t>(), std::declval<std::span<const std::byte>>(),
        std::declval<std::span<std::byte>>(), std::declval<hal::Callback<>&>())
  } -> std::convertible_to<HandleResult>;
};

namespace detail {

template <ServiceImpl Service>
class ServiceRef {
 protected:
  explicit ServiceRef(Service& service) noexcept
      : service{service} {}

  Service& service;
};

}   // namespace detail

template <hal::AsyncUart Uart, hal::CriticalSectionInterface CSI,
          ServiceImpl... Services>
  requires(sizeof...(Services) > 0)
class VrpcUart : private detail::ServiceRef<Services>... {
 public:
  static constexpr auto BufSize = std::max({Services::MinBufferSize()...})
                                  + UartDecoder::CmdFrameHeaderLength
                                  + UartDecoder::CmdFrameTailLength;

 private:
  struct Buffer {
    std::array<std::byte, BufSize> buffer;
    uint32_t                       service_id{0};
  };

  enum class RequestState {
    Empty,
    Pending,
    Handling,
    ReadyToTransmit,
    Transmitting
  };

  struct RequestSlot {
    RequestState                   state{RequestState::Empty};
    CommandRequestFrame            frame{};
    std::array<std::byte, BufSize> buffer;
  };

 public:
  explicit VrpcUart(Uart& uart, Services&... services)
      : detail::ServiceRef<Services>{services}...
      , uart{uart}
      , decoder{request_slots[0].buffer}
      , rx_callback{this, &VrpcUart::ReceiveCallback}
      , async_cmd_callbacks{
            {{this, &VrpcUart::AsyncCommandCallback<Services>}...}} {
    uart.RegisterUartReceiveCallback(rx_callback);
    uart.Receive(current_slot->buffer);
  }

  /**
   * Returns whether the vRPC UART has any requests that are pending to be
   *   handled
   * @return Whether there are any pending requests
   */
  [[nodiscard]] constexpr bool has_pending_requests() noexcept {
    return std::ranges::any_of(request_slots, [](auto& rs) {
      return rs.state == RequestState::Pending;
    });
  }

  void HandlePendingRequests() {
    for (auto& request : request_slots) {
      // Check if the request should be handled, otherwise continue
      {
        hal::CriticalSection<CSI> critical_section{};
        if (request.state == RequestState::Empty
            || request.state == RequestState::ReadyToTransmit
            || request.state == RequestState::Transmitting) {
          continue;
        }
      }

      if (request.state == RequestState::Pending) {
        (..., HandleRequest<Services>(request));
      }

      if (request.state == RequestState::ReadyToTransmit) {
        uart.Write(request.buffer);
      }
    }
  }

 private:
  template <ServiceImpl Service>
  constexpr void AsyncCommandCallback() noexcept {}

  constexpr void ReceiveCallback(std::span<std::byte> data) noexcept {
    bool restart_receive = true;
    auto result          = decoder.ConsumeBytes(data.size());

    while (!std::holds_alternative<std::monostate>(result)) {
      if (std::holds_alternative<CommandRequestFrameRef>(result)) {
        auto request = std::get<CommandRequestFrameRef>(result).get();

        if (!HasService(request.service_id)) {
          // TODO: Handle unknown service
          continue;
        }

        if (std::ranges::none_of(request_slots, [&request](auto& rs) {
              return rs.state != RequestState::Empty
                     && rs.frame.service_id == request.service_id;
            })) {
          for (auto& pending_request_slot : request_slots) {
            if (pending_request_slot.state == RequestState::Empty) {
              pending_request_slot.frame = request;
              pending_request_slot.state = RequestState::Pending;

              if (!NextBuffer()) {
                restart_receive = false;
              }
              break;
            }
          }
        } else {
          // TODO: Handle duplicate command
        }
      }
      else if (std::holds_alternative<UartDecoder::Error>(result)) {
        decoder.ResetBuffer(current_slot->buffer);
      }

      result = decoder.Decode();
    }

    if (restart_receive) {
      uart.Receive(decoder.empty_buffer());
    }
  }

  [[nodiscard]] bool NextBuffer() noexcept {
    for (auto& slot : request_slots) {
      if (slot.state == RequestState::Empty) {
        current_slot = &slot;

        if (decoder.buffer_empty()) {
          decoder.ResetBuffer(current_slot->buffer);
        } else {
          const auto undecoded = decoder.undecoded_buffer();
          std::memcpy(current_slot->buffer.data(),
                      reinterpret_cast<const void*>(undecoded.data()),
                      undecoded.size());
          decoder.ResetBuffer(
              current_slot->buffer,
              std::span{current_slot->buffer}.subspan(0, undecoded.size()));
        }

        return true;
      }
    }

    return false;
  }

  [[nodiscard]] static constexpr bool HasService(uint32_t service_id) noexcept {
    return (... || (Services::ServiceId == service_id));
  }

  template <ServiceImpl Service>
  void HandleRequest(RequestSlot& request) {
    if (request.frame.service_id != Service::ServiceId) {
      return;
    }

    request.state = RequestState::Handling;
    const auto handle_result =
        detail::ServiceRef<Service>::service.HandleCommand(
            request.frame.command_id, request.frame.payload,
            GetResponsePayloadBuffer(request.buffer),
            async_cmd_callbacks[ServiceIndex<Service>()]);
    switch (handle_result.state) {
    case HandleState::Handled:
      EncodeCommandFrame(request.buffer, request.frame.service_id,
                         request.frame.command_id, request.frame.request_id,
                         handle_result.response_payload);
      request.state = RequestState::ReadyToTransmit;
      break;
    case HandleState::HandlingAsync:
      request.state = RequestState::Handling;
      break;
    default: std::unreachable();
    }
  }

  [[nodiscard]] static constexpr std::span<std::byte>
  GetResponsePayloadBuffer(std::span<std::byte> full_buffer) noexcept {
    auto result = full_buffer.subspan(UartDecoder::CmdFrameHeaderLength);
    return result.subspan(0, result.size() - UartDecoder::CmdFrameTailLength);
  }

  template <ServiceImpl Service>
  [[nodiscard]] static consteval std::size_t ServiceIndex() noexcept {
    std::array<bool, sizeof...(Services)> is_service{
        {std::is_same_v<Service, Services>...}};

    for (std::size_t i = 0; i < is_service.size(); i++) {
      if (is_service[i]) {
        return i;
      }
    }

    std::unreachable();
    return 0;
  }

  Uart& uart;

  std::array<RequestSlot, sizeof...(Services)> request_slots{};
  RequestSlot*                                 current_slot{&request_slots[0]};

  hal::MethodCallback<VrpcUart, std::span<std::byte>> rx_callback;
  std::array<hal::MethodCallback<VrpcUart>, sizeof...(Services)>
      async_cmd_callbacks;

  UartDecoder decoder;
};

}   // namespace vrpc::uart