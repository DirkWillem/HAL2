#pragma once

#include <optional>
#include <type_traits>
#include <variant>

namespace halstd {

namespace detail {

template <std::size_t I, typename... Ts>
struct NthTypeHelper;

template <std::size_t I, typename TCur, typename... TRest>
struct NthTypeHelper<I, TCur, TRest...> {
  // static_assert(I == 0 || sizeof...(TRest) > 0, "Index out of bounds");

  using Result =
      std::conditional_t<I == 0, TCur,
                         typename NthTypeHelper<I - 1, TRest...>::Result>;
};

template <std::size_t I>
struct NthTypeHelper<I> {
  using Result = void;
};

template <std::size_t I, typename TSearch, typename... Ts>
struct FindIndexHelper {};

template <std::size_t I, typename TSearch, typename TCur, typename... TRest>
struct FindIndexHelper<I, TSearch, TCur, TRest...> {
  static constexpr std::optional<std::size_t> Result =
      std::is_same_v<TCur, TSearch>
          ? I
          : FindIndexHelper<I + 1, TSearch, TRest...>::Result;
};

template <std::size_t I, typename TSearch>
struct FindIndexHelper<I, TSearch> {
  static constexpr std::optional<std::size_t> Result = std::nullopt;
};

}   // namespace detail

/**
 * Represents a list of type
 * @tparam Ts Types
 */
template <typename... Ts>
struct Types {
  /**
   * Returns a type from the list by its index
   * @tparam I Index to get the type of, starting at 0
   */
  template <std::size_t I>
  using NthType = typename detail::NthTypeHelper<I, Ts...>::Result;

  /** Whether all types in the list are equal */
  static constexpr auto AreEqual = (... && std::is_same_v<NthType<0>, Ts>);

  /** The number of types in the list */
  static constexpr auto Count = sizeof...(Ts);

  /**
   * Finds the index of a type in the list
   * @tparam T Type to find the index of
   * @return std::optional containing the index, or nullopt if not found
   */
  template <typename T>
  static constexpr auto IndexOf() {
    return detail::FindIndexHelper<0, T, Ts...>::Result;
  }

  /**
   * Returns whether the type list contains a given type
   * @tparam T Type to check
   * @return Whether the list contains type T
   */
  template <typename T>
  static constexpr bool Contains() noexcept {
    return detail::FindIndexHelper<0, T, Ts...>::Result.has_value();
  }

  /** In the case where all types are equal, returns the singular type in the
   * list*/
  using SingleType = std::conditional_t<AreEqual, NthType<0>, void>;

  using Tuple   = std::tuple<Ts...>;
  using Variant = std::variant<Ts...>;
};

namespace detail {

template <typename Result, typename... Ts>
struct UniqueTypesHelper;

template <typename... ResultTypes, typename TCur, typename... TRest>
struct UniqueTypesHelper<Types<ResultTypes...>, TCur, TRest...> {
  using Result = std::conditional_t<
      Types<ResultTypes...>::template Contains<TCur>(),
      typename UniqueTypesHelper<Types<ResultTypes...>, TRest...>::Result,
      typename UniqueTypesHelper<Types<ResultTypes..., TCur>,
                                 TRest...>::Result>;
};

template <typename... ResultTypes>
struct UniqueTypesHelper<Types<ResultTypes...>> {
  using Result = Types<ResultTypes...>;
};

template <typename T, typename U>
struct IsSubsetHelper {};

template <typename... Ts, typename... Us>
struct IsSubsetHelper<Types<Ts...>, Types<Us...>> {
  static constexpr bool Result = (... && Types<Us...>::template Contains<Ts>());
};

}   // namespace detail

/**
 * Returns a list of types of the given types with all duplicates removed
 * @tparam Ts List of types
 */
template <typename... Ts>
using UniqueTypes = typename detail::UniqueTypesHelper<Types<>, Ts...>::Result;

/**
 * Returns whether T is a subset of U
 * @tparam T Subset list
 * @tparam U Complete list
 */
template <typename T, typename U>
inline constexpr auto TypesIsSubset = detail::IsSubsetHelper<T, U>::Result;

namespace detail {
template <typename T, typename U>
struct TransmuteVariadicHelper;

template <template <typename...> typename T, template <typename...> typename U,
          typename... Ts, typename... Us>
struct TransmuteVariadicHelper<T<Ts...>, U<Us...>> {
  using Result = U<Ts...>;
};

template <typename T, typename U>
struct IsInstantiationOfVariadicHelper : std::false_type {};

template <template <typename...> typename T, typename... Ts, typename... Us>
struct IsInstantiationOfVariadicHelper<T<Ts...>, T<Us...>> : std::true_type {};

}   // namespace detail

/**
 * Transmutes a variadic type to another variadic type with the same type
 * parameters
 * @tparam T Variadic type to transmute, with the type variables
 * @tparam U Variadic type template, instantiated with arbitrary types
 */
template <typename T, typename U>
using TransmuteVariadic =
    typename detail::TransmuteVariadicHelper<T, U>::Result;

/**
 * Returns whether T is an instantiation of variadic template U
 * @tparam T Type to check
 * @tparam U Arbitrary instantiation of the variadic template
 */
template <typename T, typename U>
inline constexpr auto IsInstantiationOfVariadic =
    detail::IsInstantiationOfVariadicHelper<T, U>::value;

static_assert(
    std::is_same_v<TransmuteVariadic<std::variant<int, bool>, Types<>>,
                   Types<int, bool>>);

/**
 * Creates a variant of the given types
 * @tparam Ts List of types to create a variant of
 */
template <typename... Ts>
using VariantOf = TransmuteVariadic<UniqueTypes<Ts...>, std::variant<>>;

static_assert(std::is_same_v<VariantOf<bool, int, bool, int, int>,
                             std::variant<bool, int>>);

}   // namespace halstd