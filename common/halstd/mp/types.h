#pragma once

#include <optional>
#include <type_traits>

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

template <typename... Ts>
struct Types {
  template <std::size_t I>
  using NthType = typename detail::NthTypeHelper<I, Ts...>::Result;

  static constexpr auto AreEqual = (... && std::is_same_v<NthType<0>, Ts>);

  template <typename T>
  static constexpr auto IndexOf() {
    return detail::FindIndexHelper<0, T, Ts...>::Result;
  }

  using SingleType = std::conditional_t<AreEqual, NthType<0>, void>;
};

}   // namespace halstd