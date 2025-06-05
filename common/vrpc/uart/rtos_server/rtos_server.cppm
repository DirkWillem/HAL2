module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

export module vrpc.uart.rtos.server;

import hstd;
import hal.abstract;

import rtos.freertos;

import nanopb;

export import vrpc.server;
export import vrpc.uart.common;
import vrpc.server.builtins;
export import vrpc.uart.server;

import vrpc.protocol.common.vrpc;
import vrpc.protocol.server_index.uart.server;

namespace vrpc::uart {

using namespace std::chrono_literals;

export template <hal::RtosUart Uart, hal::System Sys, NetworkConfig NC,
                 ServerOptions O, ServiceImpl... Services>
class RtosServer
    : private UartService<builtins::server_index::ServerIndexImpl,
                          server_index::uart::UartServerIndex>
    , private ServiceRef<server_index::uart::UartServerIndex<
          builtins::server_index::ServerIndexImpl>>
    , private ServiceRef<Services>... {
  using IndexService = server_index::uart::UartServerIndex<
      builtins::server_index::ServerIndexImpl>;

  static_assert(
      hstd::Implies(O::MultiDrop,
                    NC.topology == NetworkTopology::SingleClientMultipleServer),
      "Multi-drop UART servers are only supported for "
      "multiple-server topologies");
  static_assert(
      hstd::Implies(NC.topology == NetworkTopology::SingleClientMultipleServer,
                    O::MultiDrop),
      "Multiple-server topology requires an addressing mechanism "
      "such as multi-drop");

  static constexpr auto        HasAddress = O::MultiDrop;
  static constexpr FrameFormat FrameFmt{.has_server_addr_word = HasAddress};
  using Decoder = UartDecoder<FrameFmt>;

 public:
  /** UART buffer size */
  static constexpr auto BufSize =
      std::max({static_cast<std::size_t>(
                    nanopb::MsgDescriptor<vrpc::ServerInfo>::size),
                Services::MinBufferSize()...})
      + Decoder::CmdFrameHeaderLength + Decoder::CmdFrameTailLength;

  static constexpr std::size_t NBuiltinServices = 1;

 private:
  static constexpr auto NumReservedEventBits = 2;
  static constexpr auto MaxServiceCount =
      rtos::EventGroup::NumBits - NumReservedEventBits;

  static_assert(sizeof...(Services) + NBuiltinServices <= MaxServiceCount,
                "Too many services registered with server");

  static constexpr auto RxDoneBit = (0b1U << 0U);
  static constexpr auto TxDoneBit = (0b1U << 1U);
  static constexpr auto NumUsedBits =
      NumReservedEventBits + NBuiltinServices + sizeof...(Services);
  static constexpr auto UsedBitsMask = (0b1U << NumUsedBits) - 1;

  static consteval auto ServiceAsyncDoneBit(uint32_t service_idx) noexcept {
    return (0b1U << (service_idx + NumReservedEventBits));
  }

  static constexpr auto RequestSlotCount =
      O::RequestSlotCount.value_or(sizeof...(Services) + NBuiltinServices + 1);

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
    explicit RequestSlot(RtosServer* server) noexcept {}

    SlotState                      state{SlotState::Empty};
    CommandRequestFrame<FrameFmt>  frame{};
    std::array<std::byte, BufSize> buffer;
    std::span<const std::byte>     tx_data;
  };

  static constexpr std::array<RequestSlot, RequestSlotCount>
  CreateRequestSlots(RtosServer* server) noexcept {
    return ([server]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array<RequestSlot, RequestSlotCount>{
          {(static_cast<void>(Idxs), RequestSlot{server})...}};
    })(std::make_index_sequence<RequestSlotCount>());
  }

 public:
  explicit RtosServer(Uart& uart, Services&... services)
    requires(!HasAddress)
      : UartService<builtins::server_index::ServerIndexImpl,
                    server_index::uart::UartServerIndex>{}
      , ServiceRef<IndexService>{UartService<
            builtins::server_index::ServerIndexImpl,
            server_index::uart::UartServerIndex>::svc_uart}
      , ServiceRef<Services>{services}...
      , service_infos{{{.id         = IndexService::ServiceId,
                        .identifier = IndexService::Identifier},
                       {
                           .id         = Services::ServiceId,
                           .identifier = Services::Identifier,
                       }...}}
      , uart{uart}
      , request_slots{CreateRequestSlots(this)}
      , decoder{request_slots[0].buffer} {
    UartService<builtins::server_index::ServerIndexImpl,
                server_index::uart::UartServerIndex>::svc
        .InitializeIds(service_infos);

    StartReceiveToCurrentBuffer();
  }

  [[noreturn]] void Run() noexcept {
    while (true) {
      uint32_t   bits_to_clear{0};
      const auto set_bits =
          event_group.Wait(UsedBitsMask, 5000ms, false).value_or(0);

      // Check if there is any data to be read
      if ((set_bits & RxDoneBit) == RxDoneBit) {
        HandleReceivedData();
        bits_to_clear |= RxDoneBit;
      }

      // Check if data is transmitted
      if ((set_bits & TxDoneBit) == TxDoneBit) {
        HandleTransmitDone();
        bits_to_clear |= TxDoneBit;
      }

      // Process all slots that can be processed
      for (auto& slot : request_slots) {
        HandleSlot(slot);
      }

      event_group.ClearBits(bits_to_clear);
    }
  }

 private:
  void StartReceiveToCurrentBuffer() {
    uart.Receive(current_slot->buffer, event_group, RxDoneBit);
  }

  void HandleReceivedData() noexcept {
    // Data is always received into the current slot, so it cannot be a null
    // pointer
    [[assume(current_slot != nullptr)]];

    // Obtain the received data and feed them into the command decoder
    const auto received_data = uart.ReceivedData();
    [[assume(received_data.has_value())]];
    auto result = decoder.ConsumeBytes(received_data->size());

    // Keep on decoding frames until all complete frames are consumed
    while (!std::holds_alternative<std::monostate>(result)) {
      if (const auto* request_ptr =
              std::get_if<CommandRequestFrameRef<FrameFmt>>(&result)) {
        // Handle command request frames
        const auto& request = request_ptr->get();
        if constexpr (HasAddress) {
          if (request.server_address != this->address) {
            decoder.ResetBuffer(current_slot->buffer);
            break;
          }
        }

        if (!HasService(request.service_id)) {
          continue;
        }

        if (std::ranges::none_of(request_slots, [&request](auto& rs) {
              return rs.state != SlotState::Empty
                     && rs.frame.service_id == request.service_id;
            })) {
          current_slot->frame = request;
          current_slot->state = SlotState::PendingRequest;

          (void)NextSlot();
        } else {
          // TODO: Handle error case - 2 requests active for service
        }
      } else if (const auto& request_ptr =
                     std::get_if<ServerInfoRequestRef<FrameFmt>>(&result)) {
        // Handle server info request frames
        const auto& request = request_ptr->get();

        // If the server uses addressing, check if the frame was for this server
        if constexpr (HasAddress) {
          if (request.server_address != this->address) {
            decoder.ResetBuffer(current_slot->buffer);
            break;
          }
        }

        // Assign the frame to the current slot and set its state to pending
        current_slot->state            = SlotState::PendingGetServerInfo;
        current_slot->frame.request_id = request.request_id;

        // Activate a new slot
        (void)NextSlot();
      } else if (std::holds_alternative<typename Decoder::Error>(result)) {
        decoder.ResetBuffer(current_slot->buffer);
      }

      result = decoder.Decode();
    }

    // If there is an active slot, start receiving into the current slot
    if (current_slot != nullptr) {
      StartReceiveToCurrentBuffer();
    }
  }

  void HandleTransmitDone() noexcept {
    // Make the transmitted slot available for receiving again
    [[assume(transmitting_slot != nullptr)]];
    transmitting_slot->state = SlotState::Empty;
    transmitting_slot        = nullptr;

    // If all slots were previously busy, restart receiving into the
    // just-cleared slot
    if (current_slot == nullptr) {
      if (NextSlot()) {
        StartReceiveToCurrentBuffer();
      }
    }
  }

  void HandleSlot(RequestSlot& slot) noexcept {
    switch (slot.state) {
    case SlotState::Empty: [[fallthrough]];
    case SlotState::Transmitting: return;
    case SlotState::PendingGetServerInfo: HandleGetServerInfo(slot); break;
    case SlotState::PendingRequest: HandleRequest<IndexService>(slot); break;
    case SlotState::ReadyToTransmit: TryTransmitSlot(slot); break;
    default: break;
    }
  }

  void HandleGetServerInfo(RequestSlot& slot) noexcept {
    // Construct server info
    ServerInfo info{
        .info_version = 1,
        .service_type = MapEnum<
            std::decay_t<decltype(std::declval<ServerInfo>().service_type)>>(
            ServerServiceType::MultiService),
        .single_service_id = 0,
        .index_service_id  = IndexService::ServiceId,
        .server_name       = {},
    };

    // Encode server info to wire format
    (void)WriteProtoString(O::Name, info.server_name);

    auto payload_buffer = GetServerInfoResponsePayloadBuffer(slot.buffer);
    const auto [encode_success, encoded_payload] =
        ProtoEncode(info, payload_buffer);

    // Attempt to transmit the response
    [[assume(encode_success)]];
    slot.tx_data = EncodeInfoFrame(slot, encoded_payload);
    TryTransmitSlot(slot);
  }

  template <ServiceImpl Service>
  void HandleRequest(RequestSlot& slot) noexcept {
    if (slot.frame.service_id != Service::ServiceId) {
      return;
    }

    slot.state               = SlotState::Handling;
    const auto handle_result = ServiceRef<Service>::service.HandleCommandRtos(
        slot.frame.command_id, slot.frame.payload,
        GetCommandResponsePayloadBuffer(slot.buffer));

    switch (handle_result.state) {
    case HandleState::Handled:
      slot.tx_data = EncodeCmdFrame(slot, handle_result.response_payload);
      TryTransmitSlot(slot);
      break;
    default: break;
    }
  }

  void TryTransmitSlot(RequestSlot& slot) noexcept {
    // If there is already a slot busy transmitting, pend the current slot
    // and don't start transmission
    if (transmitting_slot != nullptr) {
      slot.state = SlotState::ReadyToTransmit;
      return;
    }

    // No slot already transmitting, start transmission of the current slot
    transmitting_slot = &slot;
    slot.state        = SlotState::Transmitting;
    uart.Write(slot.tx_data, event_group, TxDoneBit);
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

  [[nodiscard]] static constexpr std::span<std::byte>
  GetCommandResponsePayloadBuffer(std::span<std::byte> full_buffer) noexcept {
    auto result = full_buffer.subspan(Decoder::CmdFrameHeaderLength);
    return result.subspan(0, result.size() - Decoder::CmdFrameTailLength);
  }

  constexpr std::span<const std::byte>
  EncodeCmdFrame(RequestSlot&               slot,
                 std::span<const std::byte> payload) noexcept {
    if constexpr (HasAddress) {
      return EncodeCommandFrame<FrameFmt>(
          slot.buffer, this->address, slot.frame.service_id,
          slot.frame.command_id, slot.frame.request_id, payload);
    } else {
      return EncodeCommandFrame<FrameFmt>(slot.buffer, slot.frame.service_id,
                                          slot.frame.command_id,
                                          slot.frame.request_id, payload);
    }
  }

  [[nodiscard]] static constexpr std::span<std::byte>
  GetServerInfoResponsePayloadBuffer(
      std::span<std::byte> full_buffer) noexcept {
    return full_buffer.subspan(Decoder::ServerInfoFrameHeaderLength,
                               full_buffer.size()
                                   - (Decoder::ServerInfoFrameHeaderLength
                                      + Decoder::ServerInfoFrameTailLength));
  }

  constexpr std::span<const std::byte>
  EncodeInfoFrame(RequestSlot&               slot,
                  std::span<const std::byte> payload) noexcept {
    if constexpr (HasAddress) {
      // return EncodeServerInfoResponseFrame<FrameFmt>(
      //     slot.buffer, VrpcUartServerExtensions<O>::address,
      //     slot.frame.request_id, payload);
      std::unreachable();
    } else {
      return EncodeServerInfoResponseFrame<FrameFmt>(
          slot.buffer, slot.frame.request_id, payload);
    }
  }

  [[nodiscard]] static constexpr bool HasService(uint32_t service_id) noexcept {
    return IndexService::ServiceId == service_id
           || (... || (Services::ServiceId == service_id));
  }

  std::array<builtins::server_index::ServiceInfo,
             sizeof...(Services) + NBuiltinServices>
      service_infos;

  Uart& uart;

  rtos::EventGroup event_group{};

  std::array<RequestSlot, RequestSlotCount> request_slots;
  RequestSlot*                              current_slot{&request_slots[0]};
  RequestSlot*                              transmitting_slot{nullptr};

  Decoder decoder;
};

}   // namespace vrpc::uart