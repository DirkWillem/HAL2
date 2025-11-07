#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include <fp/concepts.h>

namespace math {

template <typename R>
concept UnsignedInteger =
    std::is_same_v<R, uint8_t> || std::is_same_v<R, uint16_t>
    || std::is_same_v<R, uint32_t> || std::is_same_v<R, uint64_t>;

template <typename R>
concept SignedInteger =
    std::is_same_v<R, int8_t> || std::is_same_v<R, int16_t>
    || std::is_same_v<R, int32_t> || std::is_same_v<R, int64_t>;

template <typename R>
concept Integer = UnsignedInteger<R> || SignedInteger<R>;

template <typename R>
concept Real = std::floating_point<R> || fp::FixedPointType<R>;

template <typename T>
concept Number = Integer<T> || Real<T>;

enum class NumberKind {
  Integer,
  FloatingPoint,
  FixedPoint,
};

template <Number T>
consteval NumberKind GetNumberKind() noexcept {
  if constexpr (Integer<T>) {
    return NumberKind::Integer;
  } else if constexpr (std::is_floating_point_v<T>) {
    return NumberKind::FloatingPoint;
  } else if (fp::is_fixed_point_v<T>) {
    return NumberKind::FixedPoint;
  }

  std::unreachable();
}

}   // namespace math
