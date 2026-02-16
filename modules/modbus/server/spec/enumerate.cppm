module;

#include <type_traits>
#include <utility>

export module modbus.server.spec:enumerate;

import hstd;

namespace modbus::server::spec {

/**
 * Describes a single member of an enum
 * @tparam V Enum value
 * @tparam N Member name
 */
export template <auto V, hstd::StaticString N>
struct EnumMember {
  using EnumType = std::decay_t<decltype(V)>;   //!< Enum type

  static constexpr auto Value = std::to_underlying(V);   //!< Associated value
  static constexpr auto Name  = N;                       //!< Name
};

namespace concepts {

template <typename T>
inline constexpr auto IsEnumMember = false;

template <auto V, hstd::StaticString N>
inline constexpr auto IsEnumMember<EnumMember<V, N>> = true;

/**
 * Concept that determines if a type is an instantiation of the
 * modbus::server::spec::EnumMember template
 */
template <typename T, typename E>
concept EnumMember =
    IsEnumMember<T> && (std::is_same_v<E, typename T::EnumType>);

}   // namespace concepts

/**
 * Describes an enum type
 * @tparam T Enum type
 * @tparam N Enum name
 * @tparam Ms Enum members
 */
export template <typename T, hstd::StaticString N,
                 concepts::EnumMember<T>... Ms>
struct EnumDef {
  using UnderlyingType = std::underlying_type_t<T>;   //!< Underlying type
  using Members        = hstd::Types<Ms...>;          //!< Enum members

  static constexpr auto Name = N;   //!< Enum name
};

namespace concepts {

template <typename T>
inline constexpr auto IsEnumDef = false;

template <typename E, hstd::StaticString N, typename... Ms>
inline constexpr auto IsEnumDef<EnumDef<E, N, Ms...>> = true;

export template <typename T>
concept EnumDef = IsEnumDef<std::decay_t<T>>;

/**
 * Concept that determines if a type is an instantiation of the
 * modbus::server::spec::EnumDef template
 * @tparam Impl Implementing type
 */
export template <typename Impl>
concept HasEnumDef = requires(const Impl& c_impl) {
  { c_impl.enum_def } -> EnumDef;
};

}   // namespace concepts

}   // namespace modbus::server::spec
