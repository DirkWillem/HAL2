#pragma once

#include "fix.h"

namespace fp {

template <typename T>
struct is_fixed_point : std::false_type {};

template <bool S, unsigned W, unsigned F, int Q>
struct is_fixed_point<Fix<S, W, F, Q>> : std::true_type {};

template <typename T>
inline constexpr bool is_fixed_point_v = is_fixed_point<T>::value;

template <typename T>
concept FixedPointType = is_fixed_point_v<T>;

template <typename T>
struct is_unsigned_fixed_point : std::false_type {};

template <unsigned W, unsigned F, int Q>
struct is_unsigned_fixed_point<Fix<false, W, F, Q>> : std::true_type {};

template <typename T>
inline constexpr bool is_unsigned_fixed_point_v =
    is_unsigned_fixed_point<T>::value;

template <typename T>
struct is_signed_fixed_point : std::false_type {};

template <unsigned W, unsigned F, int Q>
struct is_signed_fixed_point<Fix<true, W, F, Q>> : std::true_type {};

template <typename T>
inline constexpr bool is_signed_fixed_point_v = is_signed_fixed_point<T>::value;

}   // namespace fp