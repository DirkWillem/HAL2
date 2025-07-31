module;

#include <optional>
#include <type_traits>
#include <variant>

export module hstd:mp.types;

import :mp.helpers;

namespace hstd {

template <std::size_t I, typename... Ts>
struct NthTypeHelper;

template <std::size_t I, typename TCur, typename... TRest>
struct NthTypeHelper<I, TCur, TRest...> {
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

}   // namespace hstd

namespace hstd {

/**
 * Represents a list of type
 * @tparam Ts Types
 */
export template <typename... Ts>
struct Types {
  /**
   * Returns a type from the list by its index
   * @tparam I Index to get the type of, starting at 0
   */
  template <std::size_t I>
  using NthType = typename NthTypeHelper<I, Ts...>::Result;

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
    return FindIndexHelper<0, T, Ts...>::Result;
  }

  /**
   * Returns whether the type list contains a given type
   * @tparam T Type to check
   * @return Whether the list contains type T
   */
  template <typename T>
  static constexpr bool Contains() noexcept {
    return FindIndexHelper<0, T, Ts...>::Result.has_value();
  }

  /**
   * Applies a given function with a hstd::Marker for each of the types in the
   * list
   * @tparam F Function type
   * @param fn Function to apply
   */
  template <typename F>
    requires(... && std::invocable<F, Marker<Ts>>)
  static constexpr void ForEach(F&& fn) noexcept {
    (..., fn(hstd::Marker<Ts>()));
  }

  /** In the case where all types are equal, returns the singular type in the
   * list*/
  using SingleType = std::conditional_t<AreEqual, NthType<0>, void>;
};

}   // namespace hstd

namespace hstd {

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

}   // namespace hstd

namespace hstd {

/**
 * Returns a list of types of the given types with all duplicates removed
 * @tparam Ts List of types
 */
export template <typename... Ts>
using UniqueTypes = typename UniqueTypesHelper<Types<>, Ts...>::Result;

/**
 * Returns whether T is a subset of U
 * @tparam T Subset list
 * @tparam U Complete list
 */
export template <typename T, typename U>
inline constexpr auto TypesIsSubset = IsSubsetHelper<T, U>::Result;

/**
 * Transmutes a variadic type to another variadic type with the same type
 * parameters
 * @tparam T Variadic type to transmute, with the type variables
 * @tparam U Variadic type template, instantiated with arbitrary types
 */
export template <typename T, typename U>
using TransmuteVariadic = typename TransmuteVariadicHelper<T, U>::Result;

/**
 * Returns whether T is an instantiation of variadic template U
 * @tparam T Type to check
 * @tparam U Arbitrary instantiation of the variadic template
 */
export template <typename T, typename U>
inline constexpr auto IsInstantiationOfVariadic =
    IsInstantiationOfVariadicHelper<T, U>::value;

static_assert(
    std::is_same_v<TransmuteVariadic<std::variant<int, bool>, Types<>>,
                   Types<int, bool>>);

/**
 * Creates a variant of the given types
 * @tparam Ts List of types to create a variant of
 */
export template <typename... Ts>
using VariantOf = TransmuteVariadic<UniqueTypes<Ts...>, std::variant<>>;

static_assert(std::is_same_v<VariantOf<bool, int, bool, int, int>,
                             std::variant<bool, int>>);

export template <typename T>
inline constexpr bool IsTypes = false;

export template <typename... Ts>
inline constexpr bool IsTypes<Types<Ts...>> = true;

namespace concepts {

export template <typename T>
concept Types = IsTypes<T>;

}

template <typename... Ts>
struct ConcatTypesHelper {};

template <typename A, typename B, typename... Rest>
struct ConcatTypesHelper<A, B, Rest...> {
  using Result =
      typename ConcatTypesHelper<typename ConcatTypesHelper<A, B>::Result,
                                 Rest...>::Result;
};

template <typename... As, typename... Bs>
struct ConcatTypesHelper<Types<As...>, Types<Bs...>> {
  using Result = Types<As..., Bs...>;
};

template <typename... As>
struct ConcatTypesHelper<Types<As...>> {
  using Result = Types<As...>;
};

template <>
struct ConcatTypesHelper<> {
  using Result = Types<>;
};

export template <typename... Ts>
using ConcatTypes = typename ConcatTypesHelper<Ts...>::Result;

}   // namespace hstd