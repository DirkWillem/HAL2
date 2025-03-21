#pragma once

#include "transport.h"

namespace vrpc::uart {



namespace detail {

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O, typename Result,
          template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                    ClientTransportOptions, std::size_t> typename... Svcs>
struct ClientTupleHelper;

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O, typename... AssignedSvcs,
          template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                    ClientTransportOptions, std::size_t> typename CurSvc,
          template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                    ClientTransportOptions, std::size_t> typename... RestSvcs>
struct ClientTupleHelper<Uart, Sys, NC, O, halstd::Types<AssignedSvcs...>,
                         CurSvc, RestSvcs...> {
  static constexpr auto CurSlotId = sizeof...(AssignedSvcs);

  using AssignedCurSvc = CurSvc<Uart, Sys, NC, O, CurSlotId>;

  using Result =
      typename ClientTupleHelper<Uart, Sys, NC, O,
                                 halstd::Types<AssignedSvcs..., AssignedCurSvc>,
                                 RestSvcs...>::Result;
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O, typename... AssignedSvcs>
struct ClientTupleHelper<Uart, Sys, NC, O, halstd::Types<AssignedSvcs...>> {
  using Result = halstd::Types<AssignedSvcs...>;
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O,
          template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                    ClientTransportOptions, std::size_t> typename... Svcs>
using ClientTuple =
    typename ClientTupleHelper<Uart, Sys, NC, O, halstd::Types<>,
                               Svcs...>::Result;
template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O>
class ClientCollectionTransport {
 protected:
  explicit ClientCollectionTransport(Uart& uart) noexcept
      : transport{uart} {}
  ClientTransport<Uart, Sys, NC, O> transport;
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O, typename Svc>
class ClientCollectionClient {
 protected:
  explicit ClientCollectionClient(ClientTransport<Uart, Sys, NC, O>& transport)
      : svc{transport} {}

  Svc svc;
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O, typename Svcs>
class ClientCollectionImpl;

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O, typename... Svcs>
class ClientCollectionImpl<Uart, Sys, NC, O, halstd::Types<Svcs...>>
    : protected ClientCollectionTransport<Uart, Sys, NC, O>
    , protected ClientCollectionClient<Uart, Sys, NC, O, Svcs>... {
 protected:
  explicit ClientCollectionImpl(Uart& uart) noexcept
      : ClientCollectionTransport<Uart, Sys, NC, O>{uart}
      , ClientCollectionClient<Uart, Sys, NC, O, Svcs>{
            ClientCollectionTransport<Uart, Sys, NC, O>::transport}... {}
};

}   // namespace detail

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          ClientTransportOptions O,
          template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                    ClientTransportOptions, std::size_t> typename... Svcs>
class ClientCollection
    : protected detail::ClientCollectionImpl<
          Uart, Sys, NC, O, detail::ClientTuple<Uart, Sys, NC, O, Svcs...>> {
  using Clients = detail::ClientTuple<Uart, Sys, NC, O, Svcs...>;

  template <template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                      ClientTransportOptions, std::size_t> typename Svc>
  static consteval std::size_t GetSlotId() noexcept {
    return ([]<typename... AssignedSvcs, std::size_t... Idxs>(
                halstd::Marker<halstd::Types<AssignedSvcs...>>,
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
    })(halstd::Marker<Clients>(), std::make_index_sequence<Clients::Count>());
  }

 public:
  explicit ClientCollection(Uart& uart) noexcept
      : detail::ClientCollectionImpl<
            Uart, Sys, NC, O, detail::ClientTuple<Uart, Sys, NC, O, Svcs...>>{
            uart} {}

  template <template <hal::AsyncUart, hal::System, VrpcNetworkConfig,
                      ClientTransportOptions, std::size_t> typename Svc>
  auto& get() & noexcept {
    constexpr auto SlotId = GetSlotId<Svc>();

    return detail::ClientCollectionClient<Uart, Sys, NC, O,
                                          Svc<Uart, Sys, NC, O, SlotId>>::svc;
  }

  void HandleResponses() noexcept {
    detail::ClientCollectionTransport<Uart, Sys, NC, O>::transport
        .HandleResponses();
  }
};

}   // namespace vrpc::uart