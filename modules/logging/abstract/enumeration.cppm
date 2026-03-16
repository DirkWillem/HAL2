module;

#include <type_traits>
#include <utility>

export module logging.abstract:enumeration;

import hstd;

namespace logging {

/**
 * @brief Represents an enumeration option.
 * @tparam V Enum option value.
 * @tparam N Enum option name.
 */
export template <auto V, hstd::StaticString N>
struct EnumOption {
  using EnumType              = std::decay_t<decltype(V)>;   //!< Enum type.
  static constexpr auto Value = std::to_underlying(V);       //!< Associated value.
  static constexpr auto Name  = N;                           //!< Name.
};

namespace concepts {

template <typename T>
inline constexpr auto IsEnumOption = false;

template <auto V, hstd::StaticString N>
inline constexpr auto IsEnumOption<EnumOption<V, N>> = true;

/**
 * @brief Concept that determines if a type is an instantiation of the \c logging::EnumOption
 * template.
 */
template <typename T, typename E>
concept EnumOption = IsEnumOption<T> && (std::is_same_v<E, typename T::EnumType>);

}   // namespace concepts

/**
 * Describes an enum type
 * @tparam T Enum type.
 * @tparam N Enum name.
 * @tparam Os Enum options.
 */
export template <typename T, hstd::StaticString N, concepts::EnumOption<T>... Ms>
struct EnumDef {
  using UnderlyingType       = std::underlying_type_t<T>;   //!< Underlying type
  using Options              = hstd::Types<Ms...>;          //!< Enum options
  static constexpr auto Name = N;                           //!< Enum name
};

namespace concepts {

template <typename T>
inline constexpr auto IsEnumDef = false;

template <typename E, hstd::StaticString N, typename... Ms>
inline constexpr auto IsEnumDef<EnumDef<E, N, Ms...>> = true;

/**
 * @brief Concept that determines if a type is an instantiation of the \c logging::EnumDef template.
 */
export template <typename T>
concept EnumDef = IsEnumDef<std::decay_t<T>>;

}   // namespace concepts

export template <typename E, concepts::EnumDef D>
  requires std::is_enum_v<E>
struct Enum {
  using EnumType   = E;
  using Definition = D;

  E value;
};

namespace concepts {

template <typename T>
inline constexpr auto IsEnum = false;

template <typename E, typename D>
inline constexpr auto IsEnum<Enum<E, D>> = true;

export template <typename T>
concept Enum = IsEnum<std::decay_t<T>>;

}   // namespace concepts

}   // namespace logging
