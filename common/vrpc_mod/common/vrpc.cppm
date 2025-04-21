module;

#include <type_traits>
#include <utility>

export module vrpc.common;

export import :proto_helpers;

namespace vrpc {

/**
 * Network topology of the vRPC network
 */
export enum class NetworkTopology {
  /** Single client that communicates with a single server */
  SingleClientSingleServer,
  /** Single client that communicates with multiple servers */
  SingleClientMultipleServer,
};

export struct NetworkConfig {
  NetworkTopology topology = NetworkTopology::SingleClientMultipleServer;
};

export template <typename A, typename B>
struct EnumMapping : std::false_type {};

export template <typename A, typename B>
inline constexpr auto EnumsAreMapped =
    EnumMapping<A, B>::value || EnumMapping<B, A>::value;

export template <typename B, typename A>
constexpr auto MapEnum(A from) noexcept {
  using RealA = std::decay_t<A>;
  using RealB = std::decay_t<B>;

  static_assert(EnumsAreMapped<RealA, RealB>);

  return static_cast<RealB>(std::to_underlying(from));
}

export template <typename A, typename B>
constexpr void MapEnumInto(A from, B& into) noexcept {
  using RealA = std::decay_t<A>;
  using RealB = std::decay_t<B>;

  static_assert(EnumsAreMapped<RealA, RealB>);

  into = static_cast<RealB>(std::to_underlying(from));
}

}   // namespace vrpc