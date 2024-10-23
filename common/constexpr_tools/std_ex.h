#pragma once

#include <chrono>
#include <type_traits>

namespace stdex {

template <typename T>
struct is_ratio : std::false_type {};

template <std::intmax_t N, std::intmax_t D>
struct is_ratio<std::ratio<N, D>> : std::true_type {};

template <typename T>
inline constexpr bool is_ratio_v = is_ratio<T>::value;

template <typename T>
  requires is_ratio_v<T>
struct ratio_reciprocal_helper {
  using type = std::ratio_divide<std::ratio<1, 1>, T>;
};

template <typename T>
using ratio_reciprocal = ratio_reciprocal_helper<T>::type;

template <typename T>
concept BoolConstant = requires {
  { T::value } -> std::convertible_to<bool>;
};

namespace chrono {

template <typename T>
struct is_duration : std::false_type {};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

template <typename T>
inline constexpr bool is_duration_v = is_duration<T>::value;

}   // namespace chrono

}   // namespace stdex
