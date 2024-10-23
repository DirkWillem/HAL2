#pragma once

#include <array>
#include <concepts>
#include <tuple>
#include <utility>

namespace ct {

template <typename TIn, typename TOut, std::size_t N>
[[nodiscard]] consteval TOut
StaticMap(std::equality_comparable_with<TIn> auto value,
          std::array<std::pair<TIn, TOut>, N>     mapping) {
  for (const auto [in, out] : mapping) {
    if (value == in) {
      return out;
    }
  }

  std::unreachable();
}

}   // namespace ct