#pragma once

#include <array>
#include <concepts>
#include <tuple>
#include <utility>

namespace ct {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
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
#pragma GCC diagnostic pop

}   // namespace ct