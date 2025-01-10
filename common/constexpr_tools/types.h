#pragma once

#include "types.h"

#include <type_traits>

namespace ct {

namespace detail {

template <std::size_t I, typename... Ts>
struct NthTypeHelper;

template <std::size_t I, typename TCur, typename... TRest>
struct NthTypeHelper<I, TCur, TRest...> {
  static_assert(I == 0 || sizeof...(TRest) > 0, "Index out of bounds");

  using Result =
      std::conditional_t<I == 0, TCur,
                         typename NthTypeHelper<I - 1, TRest...>::Result>;
};

template <std::size_t I>
struct NthTypeHelper<I> {
  using Result = void;
};

}   // namespace detail

template <typename... Ts>
struct Types {
  template <std::size_t I>
  using NthType = typename detail::NthTypeHelper<I, Ts...>::Result;

  static constexpr auto AreEqual = (... && std::is_same_v<NthType<0>, Ts>);

  using SingleType = std::conditional_t<AreEqual, NthType<0>, void>;
};

}   // namespace ct