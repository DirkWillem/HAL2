module;

#include <algorithm>
#include <array>
#include <ranges>

export module hstd:array;

namespace hstd {

export template <typename T, std::size_t N>
consteval auto FillArray(T value) noexcept {
  std::array<T, N> arr{};
  std::ranges::fill(arr, value);
  return arr;
}

}   // namespace hstd
