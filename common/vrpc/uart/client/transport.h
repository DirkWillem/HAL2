#pragma once

#include <expected>

#include <halstd/atomic_helpers.h>
#include <halstd/mp/types.h>

#include <hal/system.h>
#include <hal/uart.h>

#include <vrpc/proto_helpers.h>
#include <vrpc/vrpc.h>

#include "../vrpc_uart_decoder.h"
#include "../vrpc_uart_encode.h"
#include "../vrpc_uart_frames.h"



namespace vrpc::uart {

template <typename O>
concept ClientTransportOptions = requires {
  { O::MultiDrop } -> std::convertible_to<bool>;
  { O::RxBufferSize } -> std::convertible_to<std::size_t>;
  { O::TxBufferSize } -> std::convertible_to<std::size_t>;
  { O::RequestSlotCount } -> std::convertible_to<std::size_t>;
};

template <std::size_t RSC>
struct DefaultClientTransportOptions {
  static constexpr bool        MultiDrop        = false;
  static constexpr std::size_t RxBufferSize     = 256;
  static constexpr std::size_t TxBufferSize     = 256;
  static constexpr std::size_t RequestSlotCount = RSC;
};

enum class RequestError {
  ServiceIdMismatch,
  CommandIdMismatch,
  RequestIdMismatch,
  ServerAddressMismatch,

  DecodeFailed,

  InternalError
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O>
class ClientTransport {
  using ResponseResult =
      std::expected<std::span<const std::byte>, RequestError>;

  struct RequestSlot {
    std::array<std::byte, O::TxBufferSize> tx_buffer{};
    halstd::Callback<ResponseResult>*      callback{nullptr};
    std::span<const std::byte>             tx_frame{};
    uint32_t                               server_addr{};
    uint32_t                               req_id{};
    uint32_t                               svc_id{};
    uint32_t                               cmd_id{};
    typename Sys::AtomicFlag               active{};
    typename Sys::AtomicFlag               tx_pending;
  };

  static_assert(halstd::Implies(
                    O::MultiDrop,
                    NC.topology == NetworkTopology::SingleClientMultipleServer),
                "Multi-drop UART servers are only supported for "
                "multiple-server topologies");
  static_assert(halstd::Implies(
                    NC.topology == NetworkTopology::SingleClientMultipleServer,
                    O::MultiDrop),
                "Multiple-server topology requires an addressing mechanism "
                "such as multi-drop");

  static_assert(O::RequestSlotCount > 0, "Must have at least one request slot");

 public:
  static constexpr auto UsesAddressing = O::MultiDrop;

 private:
  static constexpr FrameFormat FrameFmt{.has_server_addr_word = UsesAddressing};
  using Decoder = UartDecoder<FrameFmt>;

 public:
  explicit ClientTransport(Uart& uart) noexcept
      : uart{uart}
      , raw_rx_callback{this, &ClientTransport::RawReceiveCallback}
      , tx_callback{this, &ClientTransport::TransmitCallback} {
    uart.RegisterUartReceiveCallback(raw_rx_callback);
    uart.RegisterUartTransmitCallback(tx_callback);

    has_response.clear();
    busy.clear();
  }

  ClientTransport(const ClientTransport&)            = delete;
  ClientTransport& operator=(const ClientTransport&) = delete;

  template <typename Req>
    requires(!UsesAddressing)
  bool Request(std::size_t slot_id, uint32_t service_id, uint32_t command_id,
               const Req& request, halstd::Callback<ResponseResult>& callback) {
    if (!ActivateRequestSlot(slot_id, service_id, command_id, request,
                             callback)) {
      return false;
    }

    TrySendNextRequest();
    return true;
  }

  template <typename Req>
    requires(UsesAddressing)
  bool Request(uint32_t slot_id, uint32_t server_address, uint32_t service_id,
               uint32_t command_id, const Req& request,
               halstd::Callback<ResponseResult>& callback) {
    if (!ActivateRequestSlot(slot_id, server_address, service_id, command_id,
                             request, callback)) {
      return false;
    }

    TrySendNextRequest();
    return true;
  }

  void HandleResponses() noexcept {
    if (busy.test_and_set()) {
      if (has_response.test()) {
        halstd::ClearFlagAtExit clear_busy{busy};
        halstd::ClearFlagAtExit clear_has_response{has_response};

        if (active_slot != nullptr) {
          if (response.has_value()) {
            (*active_slot->callback)(*response);
          } else {
            (*active_slot->callback)(
                std::unexpected(RequestError::InternalError));
          }

          active_slot = nullptr;
        }
      }

      if (active_slot == nullptr) {
        TrySendNextRequest();
      }
    }
  }

 private:
  [[nodiscard]] static constexpr std::span<std::byte>
  GetCommandFramePayloadBuffer(std::span<std::byte> full_buffer) noexcept {
    auto result = full_buffer.subspan(Decoder::CmdFrameHeaderLength);
    return result.subspan(0, result.size() - Decoder::CmdFrameTailLength);
  }

  template <typename Req>
    requires(!UsesAddressing)
  [[nodiscard]] bool
  ActivateRequestSlot(std::size_t slot_id, uint32_t service_id,
                      uint32_t command_id, const Req& request,
                      halstd::Callback<ResponseResult>& callback) noexcept {
    if (slot_id >= O::RequestSlotCount) {
      std::unreachable();
    }

    auto&      slot               = request_slots[slot_id];
    const auto current_request_id = req_id;
    ++req_id;

    return halstd::ExclusiveWithAtomicFlag(slot.active, [&] -> bool {
      slot.callback = &callback;
      slot.req_id   = current_request_id;
      slot.svc_id   = service_id;
      slot.cmd_id   = command_id;

      const auto [enc_success, payload] =
          ProtoEncode(request, GetCommandFramePayloadBuffer(slot.tx_buffer));

      if (enc_success) {
        slot.tx_frame = EncodeCommandFrame(slot.tx_bufffer, slot.svc_id,
                                           slot.cmd_id, slot.req_id, payload);
        halstd::PendEvent(slot.tx_pending);
      }

      return true;
    });
  }

  template <typename Req>
    requires(UsesAddressing)
  [[nodiscard]] bool
  ActivateRequestSlot(std::size_t slot_id, uint32_t server_address,
                      uint32_t service_id, uint32_t command_id,
                      const Req&                        request,
                      halstd::Callback<ResponseResult>& callback) noexcept {
    if (slot_id >= O::RequestSlotCount) {
      std::unreachable();
    }

    auto&      slot               = request_slots[slot_id];
    const auto current_request_id = req_id;
    ++req_id;

    return halstd::ExclusiveWithAtomicFlag(
               slot.active,
               [&] -> bool {
                 slot.callback    = &callback;
                 slot.server_addr = server_address;
                 slot.req_id      = current_request_id;
                 slot.svc_id      = service_id;
                 slot.cmd_id      = command_id;

                 const auto [enc_success, payload] = ProtoEncode(
                     request, GetCommandFramePayloadBuffer(slot.tx_buffer));

                 if (enc_success) {
                   slot.tx_frame = EncodeCommandFrame<FrameFmt>(
                       slot.tx_buffer, slot.server_addr, slot.svc_id,
                       slot.cmd_id, slot.req_id, payload);
                   halstd::PendEvent(slot.tx_pending);
                 }

                 return true;
               })
        .value_or(false);
  }

  void TrySendNextRequest() noexcept {
    const auto any_sent =
        halstd::ExclusiveWithAtomicFlag(busy, [&, this] -> bool {
          for (auto& slot : request_slots) {
            if (halstd::TestAndHandleEvent(slot.tx_pending)) {
              active_slot = &slot;
              uart.Write(slot.tx_frame);
              return true;
            }
          }

          return false;
        });

    if (!any_sent) {
      busy.clear();
    }
  }

  void RawReceiveCallback(std::span<std::byte> rx_data) noexcept {
    auto result = decoder.ConsumeBytes(rx_data.size());
    while (!std::holds_alternative<std::monostate>(result)) {
      if (std::holds_alternative<CommandRequestFrameRef<FrameFmt>>(result)) {
        if (active_slot == nullptr) {
          return;
        }

        if (!has_response.test_and_set()) {
          auto request =
              std::get<CommandRequestFrameRef<FrameFmt>>(result).get();
          if constexpr (UsesAddressing) {
            if (request.server_address != active_slot->server_addr) {
              response = std::unexpected(RequestError::ServerAddressMismatch);
              return;
            }
          }

          if (request.service_id != active_slot->svc_id) {
            response = std::unexpected(RequestError::ServiceIdMismatch);
            return;
          }
          if (request.command_id != active_slot->cmd_id) {
            response = std::unexpected(RequestError::CommandIdMismatch);
            (*active_slot->callback)(
                std::unexpected(RequestError::CommandIdMismatch));
            active_slot = nullptr;
            return;
          }
          if (request.request_id != active_slot->req_id) {
            response = std::unexpected(RequestError::RequestIdMismatch);
            return;
          }

          response = request.payload;
        }
      }

      result = decoder.Decode();
    }
  }

  void TransmitCallback() noexcept { uart.Receive(rx_buffer); }

  Uart& uart;

  typename Sys::AtomicFlag busy{};
  typename Sys::AtomicFlag has_response{};

  uint32_t req_id{0};

  RequestSlot* active_slot{nullptr};

  std::span<const std::byte>             rx_data{};
  std::array<std::byte, O::RxBufferSize> rx_buffer{};
  Decoder                                decoder{rx_buffer};

  std::array<RequestSlot, O::RequestSlotCount> request_slots{};

  halstd::MethodCallback<ClientTransport, std::span<std::byte>> raw_rx_callback;
  halstd::MethodCallback<ClientTransport>                       tx_callback;

  std::optional<ResponseResult> response{};
};

}
