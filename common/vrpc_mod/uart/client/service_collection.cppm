module;

#include <utility>

export module vrpc.uart.client:service_collection;

import hstd;
import hal.abstract;

import vrpc.common;
import vrpc.uart.common;

import :transport;

namespace vrpc::uart {

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, typename Result,
          template <hal::AsyncUart, hal::System, NetworkConfig,
                    ClientTransportOptions, std::size_t> typename... Svcs>
struct ClientTupleHelper;

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, typename... AssignedSvcs,
          template <hal::AsyncUart, hal::System, NetworkConfig,
                    ClientTransportOptions, std::size_t> typename CurSvc,
          template <hal::AsyncUart, hal::System, NetworkConfig,
                    ClientTransportOptions, std::size_t> typename... RestSvcs>
struct ClientTupleHelper<Uart, Sys, NC, O, hstd::Types<AssignedSvcs...>, CurSvc,
                         RestSvcs...> {
  static constexpr auto CurSlotId = sizeof...(AssignedSvcs);

  using AssignedCurSvc = CurSvc<Uart, Sys, NC, O, CurSlotId>;

  using Result =
      typename ClientTupleHelper<Uart, Sys, NC, O,
                                 hstd::Types<AssignedSvcs..., AssignedCurSvc>,
                                 RestSvcs...>::Result;
};

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, typename... AssignedSvcs>
struct ClientTupleHelper<Uart, Sys, NC, O, hstd::Types<AssignedSvcs...>> {
  using Result = hstd::Types<AssignedSvcs...>;
};

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O,
          template <hal::AsyncUart, hal::System, NetworkConfig,
                    ClientTransportOptions, std::size_t> typename... Svcs>
using ClientTuple = typename ClientTupleHelper<Uart, Sys, NC, O, hstd::Types<>,
                                               Svcs...>::Result;
template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O>
class ClientCollectionTransport {
 protected:
  explicit ClientCollectionTransport(Uart& uart) noexcept
      : transport{uart} {}
  ClientTransport<Uart, Sys, NC, O> transport;
};

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, typename Svc>
class ClientCollectionClient {
 protected:
  explicit ClientCollectionClient(ClientTransport<Uart, Sys, NC, O>& transport)
      : svc{transport} {}

  Svc svc;
};

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, typename Svcs>
class ClientCollectionImpl;

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, typename... Svcs>
class ClientCollectionImpl<Uart, Sys, NC, O, hstd::Types<Svcs...>>
    : protected ClientCollectionTransport<Uart, Sys, NC, O>
    , protected ClientCollectionClient<Uart, Sys, NC, O, Svcs>... {
 protected:
  explicit ClientCollectionImpl(Uart& uart) noexcept
      : ClientCollectionTransport<Uart, Sys, NC, O>{uart}
      , ClientCollectionClient<Uart, Sys, NC, O, Svcs>{
            ClientCollectionTransport<Uart, Sys, NC, O>::transport}... {}
};

export template <
    hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
    template <std::size_t> typename OT,
    template <hal::AsyncUart, hal::System, NetworkConfig,
              ClientTransportOptions, std::size_t> typename... Svcs>
  requires ClientTransportOptions<OT<1>>
class ClientCollection
    : protected ClientCollectionImpl<
          Uart, Sys, NC, OT<sizeof...(Svcs)>,
          ClientTuple<Uart, Sys, NC, OT<sizeof...(Svcs)>, Svcs...>> {
  using O       = OT<sizeof...(Svcs)>;
  using Clients = ClientTuple<Uart, Sys, NC, O, Svcs...>;

  template <template <typename, typename, ::vrpc::NetworkConfig, typename,
                      std::size_t> typename Svc>
  static consteval std::size_t GetSlotId() noexcept {
    return ([]<typename... AssignedSvcs, std::size_t... Idxs>(
                hstd::Marker<hstd::Types<AssignedSvcs...>>,
                std::index_sequence<Idxs...>) {
      std::array<bool, Clients::Count> Matches{
          std::is_same_v<std::decay_t<AssignedSvcs>,
                         std::decay_t<Svc<Uart, Sys, NC, O, Idxs>>>...};

      for (std::size_t i = 0; i < Matches.size(); ++i) {
        if (Matches[i]) {
          return i;
        }
      }

      std::unreachable();
    })(hstd::Marker<Clients>(), std::make_index_sequence<Clients::Count>());
  }

 public:
  explicit ClientCollection(Uart& uart) noexcept
      : ClientCollectionImpl<Uart, Sys, NC, O,
                             ClientTuple<Uart, Sys, NC, O, Svcs...>>{uart} {}

  template <template <typename, typename, ::vrpc::NetworkConfig, typename,
                      std::size_t> typename Svc>
  auto& get() & noexcept {
    constexpr auto SlotId = GetSlotId<Svc>();

    return ClientCollectionClient<Uart, Sys, NC, O,
                                  Svc<Uart, Sys, NC, O, SlotId>>::svc;
  }

  template <template <typename, typename, ::vrpc::NetworkConfig, typename,
                      std::size_t> typename Svc>
  using ServiceImplementation = Svc<Uart, Sys, NC, O, GetSlotId<Svc>()>;

  void HandleResponses() noexcept {
    ClientCollectionTransport<Uart, Sys, NC, O>::transport.HandleResponses();
  }
};

}   // namespace vrpc::uart