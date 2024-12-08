#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <limits>

namespace ct {

template <typename T, std::size_t N>
[[nodiscard]] constexpr T Max(const std::array<T, N>& arr) {
  T result{std::numeric_limits<T>::min()};

  for (auto v : arr) {
    result = std::max(v, result);
  }

  return result;
}

template <std::integral T, std::size_t N>
[[nodiscard]] constexpr T Min(const std::array<T, N>& arr) {
  T result{std::numeric_limits<T>::max()};

  for (auto v : arr) {
    result = std::min(v, result);
  }

  return result;
}

[[nodiscard]] constexpr bool
IsPowerOf2(std::unsigned_integral auto v) noexcept {
  return v && ((v & (v - 1)) == 0);
}

template <std::integral T>
[[nodiscard]] constexpr unsigned NumDigits(T value, T base = 10) {
  unsigned n_digits = 1;
  T        compare  = base;

  while (compare <= value) {
    // Handle overflow
    if (static_cast<T>(compare * base) < compare) {
      return n_digits;
    }

    n_digits++;
    compare *= base;
  }

  return n_digits;
}

constexpr float Pow2(int exp) {
  if (exp > 0) {
    const auto Res = static_cast<uint64_t>(0b1U) << static_cast<unsigned>(exp);
    return static_cast<float>(Res);
  } else if (exp < 0) {
    const auto Div = static_cast<uint64_t>(0b1U) << static_cast<unsigned>(-exp);
    return 1.F / static_cast<float>(Div);
  } else {
    return 1.F;
  }
}

template <std::unsigned_integral T>
constexpr T FirstMultipleAfter(T multiplier, T x) noexcept {
  if (x % multiplier == 0) {
    return x;
  } else {
    return multiplier * (1 + x / multiplier);
  }
}

}   // namespace ct
