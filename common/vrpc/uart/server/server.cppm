module;

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

export module vrpc.uart.server;

import hstd;
import hal.abstract;

import nanopb;

export import vrpc.server;
export import vrpc.uart.common;
import vrpc.server.builtins;

import vrpc.protocol.common.vrpc;
import vrpc.protocol.server_index.uart.server;

namespace vrpc::uart {

export template <typename O>
concept ServerOptions = requires {
  { O::RequestSlotCount } -> std::convertible_to<std::optional<std::size_t>>;
  { O::Name } -> std::convertible_to<std::string_view>;
  { O::MultiDrop } -> std::convertible_to<bool>;

  { O::AutoStart } -> std::convertible_to<bool>;

  requires O::Name.length() <= 32;
};

export struct DefaultServerOptions {
  static constexpr std::optional<std::size_t> RequestSlotCount = std::nullopt;
  static constexpr std::string_view           Name             = "vRPC App";
  static constexpr bool                       MultiDrop        = false;
  static constexpr bool                       AutoStart        = true;
};

template <ServerOptions O>
class VrpcUartServerExtensions {};

template <ServerOptions O>
  requires O::MultiDrop
class VrpcUartServerExtensions<O> {
 public:
  constexpr void ChangeAddress(uint32_t new_address) noexcept {
    address = new_address;
  }

 protected:
  explicit VrpcUartServerExtensions(uint32_t address) noexcept
      : address{address} {}

  uint32_t address;
};

export template <typename Impl>
concept ServiceImpl = requires(Impl& impl) {
  { Impl::ServiceId } -> std::convertible_to<uint32_t>;

  requires hstd::IsConstantExpression<Impl::MinBufferSize()>();
  { Impl::MinBufferSize() } -> std::convertible_to<std::size_t>;

  {
    impl.HandleCommand(std::declval<uint32_t>(),
                       std::declval<std::span<const std::byte>>(),
                       std::declval<std::span<std::byte>>(),
                       std::declval<hstd::Callback<HandleResult>&>())
  } -> std::convertible_to<HandleResult>;
};

export template <typename Service, template <typename> typename UartImpl>
struct UartService {
  template <typename... Args>
  explicit UartService(Args&&... args)
      : svc{std::forward<Args>(args)...}
      , svc_uart{svc} {}

  Service           svc;
  UartImpl<Service> svc_uart;
};

export template <ServiceImpl Service>
class ServiceRef {
 protected:
  explicit ServiceRef(Service& service) noexcept
      : service{service} {}

  Service& service;
};

export template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ServerOptions O, ServiceImpl... Services>
  requires(sizeof...(Services) > 0)
/**
 * vRPC UART server implementation. Can operate in both a single sever, single
 * client mode, and a multi-drop config
 * @tparam Uart UART implementation
 * @tparam Sys System implementation
 * @tparam NC Network Configuration
 * @tparam O Server options
 * @tparam Services Services to be implemented by the server
 */
class Server
    : private UartService<builtins::server_index::ServerIndexImpl,
                          server_index::uart::UartServerIndex>
    , private ServiceRef<server_index::uart::UartServerIndex<
          builtins::server_index::ServerIndexImpl>>
    , private ServiceRef<Services>...
    , public VrpcUartServerExtensions<O> {
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
    explicit RequestSlot(Server* server) noexcept
        : async_cmd_callback{server} {}

    SlotState                      state{SlotState::Empty};
    CommandRequestFrame<FrameFmt>  frame{};
    std::array<std::byte, BufSize> buffer;
    std::span<const std::byte>     tx_data;
    hstd::BoundDynamicMethodCallback<Server, hstd::Types<RequestSlot*>,
                                     hstd::Callback<HandleResult>>
        async_cmd_callback;
  };

  static constexpr std::array<RequestSlot, RequestSlotCount>
  CreateRequestSlots(Server* server) noexcept {
    return ([server]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array<RequestSlot, RequestSlotCount>{
          {(static_cast<void>(Idxs), RequestSlot{server})...}};
    })(std::make_index_sequence<RequestSlotCount>());
  }

 public:
  explicit Server(Uart& uart, Services&... services)
    requires(!HasAddress)
      : UartService<builtins::server_index::ServerIndexImpl,
                    server_index::uart::UartServerIndex>{}
      , ServiceRef<IndexService>{UartService<
            builtins::server_index::ServerIndexImpl,
            server_index::uart::UartServerIndex>::svc_uart}
      , ServiceRef<Services>{services}...
      , VrpcUartServerExtensions<O>{}
      , service_infos{{{.id         = IndexService::ServiceId,
                        .identifier = IndexService::Identifier},
                       {
                           .id         = Services::ServiceId,
                           .identifier = Services::Identifier,
                       }...}}
      , uart{uart}
      , request_slots{CreateRequestSlots(this)}
      , rx_callback{this, &Server::ReceiveCallback}
      , tx_callback{this, &Server::TransmitCallback}
      , decoder{request_slots[0].buffer} {
    uart.RegisterUartReceiveCallback(rx_callback);
    uart.RegisterUartTransmitCallback(tx_callback);

    UartService<builtins::server_index::ServerIndexImpl,
                server_index::uart::UartServerIndex>::svc
        .InitializeIds(service_infos);

    StartReceiveToCurrentBuffer();
  }

  explicit Server(Uart& uart, uint32_t addr, Services&... services)
    requires HasAddress
      : UartService<builtins::server_index::ServerIndexImpl,
                    server_index::uart::UartServerIndex>{}
      , ServiceRef<IndexService>{UartService<
            builtins::server_index::ServerIndexImpl,
            server_index::uart::UartServerIndex>::svc_uart}
      , ServiceRef<Services>{services}...
      , VrpcUartServerExtensions<O>{addr}
      , service_infos{{{.id         = IndexService::ServiceId,
                        .identifier = IndexService::Identifier},
                       {
                           .id         = Services::ServiceId,
                           .identifier = Services::Identifier,
                       }...}}
      , uart{uart}
      , request_slots{CreateRequestSlots(this)}
      , rx_callback{this, &Server::ReceiveCallback}
      , tx_callback{this, &Server::TransmitCallback}
      , decoder{request_slots[0].buffer} {
    uart.RegisterUartReceiveCallback(rx_callback);
    uart.RegisterUartTransmitCallback(tx_callback);

    UartService<builtins::server_index::ServerIndexImpl,
                server_index::uart::UartServerIndex>::svc
        .InitializeIds(service_infos);

    if constexpr (O::AutoStart) {
      StartReceiveToCurrentBuffer();
    }
  }

  ~Server() {
    uart.ClearUartReceiveCallback();
    uart.ClearUartTransmitCallback();
  }

  /**
   * Starts the server
   */
  void Start() noexcept { StartReceiveToCurrentBuffer(); }

  /**
   * Returns whether the vRPC UART has any requests that are pending to be
   *   handled
   * @return Whether there are any pending requests
   */
  [[nodiscard]] constexpr bool has_pending_requests() const noexcept {
    return std::ranges::any_of(request_slots, [](auto& rs) {
      return rs.state == SlotState::PendingRequest;
    });
  }

  void HandlePendingRequests() noexcept {
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
  constexpr void AsyncCommandCallback(RequestSlot* slot,
                                      HandleResult handle_result) noexcept {
    slot->tx_data = EncodeCmdFrame(*slot, handle_result.response_payload);
    slot->state   = SlotState::ReadyToTransmit;
  }

  constexpr void ReceiveCallback(std::span<std::byte> data) noexcept {
    bool restart_receive = true;
    auto result          = decoder.ConsumeBytes(data.size());

    while (!std::holds_alternative<std::monostate>(result)) {
      if (std::holds_alternative<CommandRequestFrameRef<FrameFmt>>(result)) {
        auto request = std::get<CommandRequestFrameRef<FrameFmt>>(result).get();

        if constexpr (HasAddress) {
          if (request.server_address != this->address) {
            decoder.ResetBuffer(current_slot->buffer);
            break;
          }
        }

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
      } else if (std::holds_alternative<ServerInfoRequestRef<FrameFmt>>(
                     result)) {
        const auto& req =
            std::get<ServerInfoRequestRef<FrameFmt>>(result).get();

        if constexpr (HasAddress) {
          if (req.server_address != this->address) {
            decoder.ResetBuffer(current_slot->buffer);
            break;
          }
        }

        if (current_slot != nullptr) {
          current_slot->state            = SlotState::PendingGetServerInfo;
          current_slot->frame.request_id = req.request_id;
          if (!NextSlot()) {
            restart_receive = false;
          }
        }
      } else if (std::holds_alternative<typename Decoder::Error>(result)) {
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
  void HandleRequest(RequestSlot& slot) {
    if (slot.frame.service_id != Service::ServiceId) {
      return;
    }

    slot.state = SlotState::Handling;
    slot.async_cmd_callback.RebindUnguarded(
        &Server::AsyncCommandCallback<Service>, std::make_tuple(&slot));

    const auto handle_result = ServiceRef<Service>::service.HandleCommand(
        slot.frame.command_id, slot.frame.payload,
        GetCommandResponsePayloadbuffer(slot.buffer), slot.async_cmd_callback);

    switch (handle_result.state) {
    case HandleState::Handled:
      slot.tx_data = EncodeCmdFrame(slot, handle_result.response_payload);
      slot.state   = SlotState::ReadyToTransmit;
      break;
    case HandleState::HandlingAsync: slot.state = SlotState::Handling; break;
    default: std::unreachable();
    }
  }

  void HandleGetServerInfo(RequestSlot& slot) {
    auto payload_buffer = GetServerInfoResponsePayloadBuffer(slot.buffer);

    ServerInfo info{
        .info_version = 1,
        .service_type = MapEnum<
            std::decay_t<decltype(std::declval<ServerInfo>().service_type)>>(
            ServerServiceType::MultiService),
        .single_service_id = 0,
        .index_service_id  = IndexService::ServiceId,
        .server_name       = {},
    };

    (void)WriteProtoString(O::Name, info.server_name);

    const auto [encode_success, encoded_payload] =
        ProtoEncode(info, payload_buffer);

    if (encode_success) {
      slot.tx_data = EncodeInfoFrame(slot, encoded_payload);
      slot.state   = SlotState::ReadyToTransmit;
    } else {
      slot.state = SlotState::Empty;
    }
  }

  [[nodiscard]] static constexpr std::span<std::byte>
  GetCommandResponsePayloadbuffer(std::span<std::byte> full_buffer) noexcept {
    auto result = full_buffer.subspan(Decoder::CmdFrameHeaderLength);
    return result.subspan(0, result.size() - Decoder::CmdFrameTailLength);
  }

  [[nodiscard]] static constexpr std::span<std::byte>
  GetServerInfoResponsePayloadBuffer(
      std::span<std::byte> full_buffer) noexcept {
    return full_buffer.subspan(Decoder::ServerInfoFrameHeaderLength,
                               full_buffer.size()
                                   - (Decoder::ServerInfoFrameHeaderLength
                                      + Decoder::ServerInfoFrameTailLength));
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

  constexpr std::span<const std::byte>
  EncodeInfoFrame(RequestSlot&               slot,
                  std::span<const std::byte> payload) noexcept {
    if constexpr (HasAddress) {
      return EncodeServerInfoResponseFrame<FrameFmt>(
          slot.buffer, VrpcUartServerExtensions<O>::address,
          slot.frame.request_id, payload);
    } else {
      return EncodeServerInfoResponseFrame<FrameFmt>(
          slot.buffer, slot.frame.request_id, payload);
    }
  }

  std::array<builtins::server_index::ServiceInfo,
             sizeof...(Services) + NBuiltinServices>
      service_infos;

  Uart& uart;

  std::array<RequestSlot, RequestSlotCount> request_slots;
  RequestSlot*                              current_slot{&request_slots[0]};
  RequestSlot*                              transmitting_slot{nullptr};

  hstd::MethodCallback<Server, std::span<std::byte>> rx_callback;
  hstd::MethodCallback<Server>                       tx_callback;

  Decoder decoder;

  typename Sys::AtomicFlag transmitting{};
  typename Sys::AtomicFlag receiving{};
};

export template <typename Impl>
concept IsServer = requires(Impl& impl, const Impl& c_impl) {
  { impl.Start() };
  { c_impl.has_pending_requests() } -> std::convertible_to<bool>;
  { impl.HandlePendingRequests() };
};

}   // namespace vrpc::uart