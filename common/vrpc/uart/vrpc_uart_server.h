#pragma once

#include "vrpc_uart.h"

#include <vrpc/builtins/server_index.h>
#include <vrpc/generated/services/server_index_uart.h>

namespace vrpc::uart {

template <typename O>
concept VrpcUartServerOptions = requires {
  { O::RequestSlotCount } -> std::convertible_to<std::optional<std::size_t>>;
  { O::Name } -> std::convertible_to<std::string_view>;
  requires O::Name.length() <= 32;
};

struct DefaultVrpcUartServerOptions {
  static constexpr std::optional<std::size_t> RequestSlotCount = std::nullopt;
  static constexpr std::string_view           Name             = "vRPC App";
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcUartServerOptions O,
          ServiceImpl... Services>
  requires(sizeof...(Services) > 0)
class VrpcUartServer
    : private UartService<server_index::ServerIndexImpl,
                          server_index::uart::UartServerIndex>
    , private detail::ServiceRef<
          server_index::uart::UartServerIndex<server_index::ServerIndexImpl>>
    , private detail::ServiceRef<Services>... {
  using IndexService =
      server_index::uart::UartServerIndex<server_index::ServerIndexImpl>;

 public:
  static constexpr auto BufSize =
      std::max({static_cast<std::size_t>(
                    nanopb::MessageDescriptor<vrpc::ServerInfo>::size),
                Services::MinBufferSize()...})
      + UartDecoder::CmdFrameHeaderLength + UartDecoder::CmdFrameTailLength;

  static constexpr std::size_t NBuiltinServices = 1;

 private:
  struct Buffer {
    std::array<std::byte, BufSize> buffer;
    uint32_t                       service_id{0};
  };

  enum class SlotState {
    Empty,
    PendingRequest,
    PendingGetServerInfo,
    Handling,
    ReadyToTransmit,
    Transmitting
  };

  struct RequestSlot {
    SlotState                      state{SlotState::Empty};
    CommandRequestFrame            frame{};
    std::array<std::byte, BufSize> buffer;
    std::span<const std::byte>     tx_data;
  };

 public:
  explicit VrpcUartServer(Uart& uart, Services&... services)
      : UartService<server_index::ServerIndexImpl,
                    server_index::uart::UartServerIndex>{}
      , detail::ServiceRef<IndexService>{UartService<
            server_index::ServerIndexImpl,
            server_index::uart::UartServerIndex>::svc_uart}
      , detail::ServiceRef<Services>{services}...
      , service_infos{{{.id         = IndexService::ServiceId,
                        .identifier = IndexService::Identifier},
                       {
                           .id         = Services::ServiceId,
                           .identifier = Services::Identifier,
                       }...}}
      , uart{uart}
      , decoder{request_slots[0].buffer}
      , rx_callback{this, &VrpcUartServer::ReceiveCallback}
      , tx_callback{this, &VrpcUartServer::TransmitCallback}
      , async_cmd_callbacks{
            {{this, &VrpcUartServer::AsyncCommandCallback<IndexService>},
             {this, &VrpcUartServer::AsyncCommandCallback<Services>}...}} {
    uart.RegisterUartReceiveCallback(rx_callback);
    uart.RegisterUartTransmitCallback(tx_callback);

    UartService<server_index::ServerIndexImpl,
                server_index::uart::UartServerIndex>::svc
        .InitializeIds(service_infos);

    StartReceiveToCurrentBuffer();
  }

  /**
   * Returns whether the vRPC UART has any requests that are pending to be
   *   handled
   * @return Whether there are any pending requests
   */
  [[nodiscard]] constexpr bool has_pending_requests() noexcept {
    return std::ranges::any_of(request_slots, [](auto& rs) {
      return rs.state == SlotState::PendingRequest;
    });
  }

  void HandlePendingRequests() {
    for (auto& request : request_slots) {
      // Check if the request should be handled, otherwise continue
      {
        hal::CriticalSection<typename Sys::CriticalSectionInterface>
            critical_section{};
        if (request.state == SlotState::Empty
            || request.state == SlotState::ReadyToTransmit
            || request.state == SlotState::Transmitting) {
          continue;
        }
      }

      if (request.state == SlotState::PendingRequest) {
        HandleRequest<IndexService>(request);
        (..., HandleRequest<Services>(request));
      } else if (request.state == SlotState::PendingGetServerInfo) {
        HandleGetServerInfo(request);
      }
    }

    TransmitNextResponse();
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
              return rs.state != SlotState::Empty
                     && rs.frame.service_id == request.service_id;
            })) {
          if (current_slot != nullptr) {
            current_slot->frame = request;
            current_slot->state = SlotState::PendingRequest;

            if (!NextSlot()) {
              restart_receive = false;
            }
          }
        }
      } else if (std::holds_alternative<ServerInfoRequestRef>(result)) {
        if (current_slot != nullptr) {
          const auto& req     = std::get<ServerInfoRequestRef>(result).get();
          current_slot->state = SlotState::PendingGetServerInfo;
          current_slot->frame.request_id = req.request_id;
          if (!NextSlot()) {
            restart_receive = false;
          }
        }
      } else if (std::holds_alternative<UartDecoder::Error>(result)) {
        decoder.ResetBuffer(current_slot->buffer);
      }

      result = decoder.Decode();
    }

    if (restart_receive) {
      StartReceiveToCurrentBuffer();
    }
  }

  constexpr void TransmitCallback() noexcept {
    transmitting.clear();
    if (transmitting_slot != nullptr) {
      transmitting_slot->state = SlotState::Empty;
    }

    bool restart_receive = false;
    {
      hal::CriticalSection<typename Sys::CriticalSectionInterface>
          critical_section{};
      restart_receive = !receiving.test_and_set();
      receiving.clear();
    }

    if (restart_receive) {
      if (NextSlot()) {
        StartReceiveToCurrentBuffer();
      }
    }

    TransmitNextResponse();
  }

  /**
   * Starts receiving bytes into the current buffer
   */
  void StartReceiveToCurrentBuffer() {
    (void)receiving.test_and_set();
    uart.Receive(current_slot->buffer);
  }

  /**
   * Transmits the next response of a request slot that has state
   * ReadyToTransmit
   */
  void TransmitNextResponse() {
    for (auto& request : request_slots) {
      if (request.state == SlotState::ReadyToTransmit) {
        if (!transmitting.test_and_set()) {
          transmitting_slot = &request;
          uart.Write(request.tx_data);
        } else {
          return;
        }
      }
    }
  }

  /**
   * Selects the next request slot, or does nothing if no slots are available
   * @return Whether a new slot was selected
   */
  [[nodiscard]] bool NextSlot() noexcept {
    current_slot = nullptr;

    for (auto& slot : request_slots) {
      if (slot.state == SlotState::Empty) {
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
    return IndexService::ServiceId == service_id
           || (... || (Services::ServiceId == service_id));
  }

  template <ServiceImpl Service>
  void HandleRequest(RequestSlot& request) {
    if (request.frame.service_id != Service::ServiceId) {
      return;
    }

    request.state = SlotState::Handling;
    const auto handle_result =
        detail::ServiceRef<Service>::service.HandleCommand(
            request.frame.command_id, request.frame.payload,
            GetCommandResponsePayloadbuffer(request.buffer),
            async_cmd_callbacks[ServiceIndex<Service>()]);
    switch (handle_result.state) {
    case HandleState::Handled:
      request.tx_data = EncodeCommandFrame(
          request.buffer, request.frame.service_id, request.frame.command_id,
          request.frame.request_id, handle_result.response_payload);
      request.state = SlotState::ReadyToTransmit;
      break;
    case HandleState::HandlingAsync: request.state = SlotState::Handling; break;
    default: std::unreachable();
    }
  }

  void HandleGetServerInfo(RequestSlot& request) {
    auto payload_buffer = GetServerInfoResponsePayloadBuffer(request.buffer);

    vrpc::ServerInfo info{
        .info_version = 1,
        .service_type = static_cast<vrpc_ServerServiceType>(
            vrpc::ServerServiceType::MultiService),
        .single_service_id = 0,
        .index_service_id  = IndexService::ServiceId,
    };

    WriteProtoString(O::Name, info.server_name);

    const auto [encode_success, encoded_payload] =
        ProtoEncode(info, payload_buffer);

    if (encode_success) {
      request.tx_data = EncodeServerInfoResponseFrame(
          request.buffer, request.frame.request_id, encoded_payload);
      request.state = SlotState::ReadyToTransmit;
    } else {
      request.state = SlotState::Empty;
    }
  }

  [[nodiscard]] static constexpr std::span<std::byte>
  GetCommandResponsePayloadbuffer(std::span<std::byte> full_buffer) noexcept {
    auto result = full_buffer.subspan(UartDecoder::CmdFrameHeaderLength);
    return result.subspan(0, result.size() - UartDecoder::CmdFrameTailLength);
  }

  [[nodiscard]] static constexpr std::span<std::byte>
  GetServerInfoResponsePayloadBuffer(
      std::span<std::byte> full_buffer) noexcept {
    return full_buffer.subspan(
        UartDecoder::ServerInfoFrameHeaderLength,
        full_buffer.size()
            - (UartDecoder::ServerInfoFrameHeaderLength
               + UartDecoder::ServerInfoFrameTailLength));
  }

  template <ServiceImpl Service>
  [[nodiscard]] static consteval std::size_t ServiceIndex() noexcept {
    std::array<bool, sizeof...(Services) + NBuiltinServices> is_service{
        {std::is_same_v<Service, IndexService>,
         std::is_same_v<Service, Services>...}};

    for (std::size_t i = 0; i < is_service.size(); i++) {
      if (is_service[i]) {
        return i;
      }
    }

    std::unreachable();
    return 0;
  }

  std::array<server_index::ServiceInfo, sizeof...(Services) + NBuiltinServices>
      service_infos;

  Uart& uart;

  std::array<RequestSlot, O::RequestSlotCount.value_or(sizeof...(Services)
                                                       + NBuiltinServices + 1)>
               request_slots{};
  RequestSlot* current_slot{&request_slots[0]};
  RequestSlot* transmitting_slot{nullptr};

  hal::MethodCallback<VrpcUartServer, std::span<std::byte>> rx_callback;
  hal::MethodCallback<VrpcUartServer>                       tx_callback;
  std::array<hal::MethodCallback<VrpcUartServer>,
             sizeof...(Services) + NBuiltinServices>
      async_cmd_callbacks;

  UartDecoder decoder;

  std::atomic_flag transmitting{};
  std::atomic_flag receiving{};
};

}   // namespace vrpc::uart