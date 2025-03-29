#pragma once

namespace vrpc {

/**
 * Network topology of the vRPC network
 */
enum class NetworkTopology {
  /** Single client that communicates with a single server */
  SingleClientSingleServer,
  /** Single client that communicates with multiple servers */
  SingleClientMultipleServer,
};

struct NetworkConfig {
  NetworkTopology topology = NetworkTopology::SingleClientMultipleServer;
};

template <typename A, typename B>
struct EnumMapping : std::false_type {};

template <typename A, typename B>
inline constexpr auto EnumsAreMapped =
    EnumMapping<A, B>::value || EnumMapping<B, A>::value;

template <typename B, typename A>
constexpr auto MapEnum(A from) noexcept {
  using RealA = std::decay_t<A>;
  using RealB = std::decay_t<B>;

  static_assert(EnumsAreMapped<RealA, RealB>);

  return static_cast<RealB>(std::to_underlying(from));
}

}   // namespace vrpc