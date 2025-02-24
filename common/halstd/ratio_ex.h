#pragma once

#include <ratio>

namespace halstd {

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
using ratio_reciprocal = typename ratio_reciprocal_helper<T>::type;

template <typename T>
/** Concept for std::ratio */
concept Ratio = halstd::is_ratio_v<T>;

}   // namespace halstd