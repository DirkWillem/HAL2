#pragma once

#include <type_traits>

namespace ct {

template <std::size_t V>
using Size = std::integral_constant<std::size_t, V>;

template<typename T>
concept Integer = std::is_unsigned_v<T> || std::is_signed_v<T>;

template<typename T, typename U>
struct Map_t {
  using type = U;
};

template<typename T, typename U>
using Map = Map_t<T, U>::type;

}   // namespace ct