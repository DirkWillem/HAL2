module;

#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <ranges>
#include <span>

export module hstd:memory;

import :endian;

namespace hstd {

export template <std::endian E, typename T>
constexpr auto ToByteArray(const T& data) noexcept {
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(
      ConvertToEndianness<E>(data));
}

export template <std::endian E, typename T, std::size_t N>
constexpr auto ToByteArray(const std::array<T, N>& data) noexcept {
  auto result = std::bit_cast<std::array<std::byte, sizeof(T) * N>>(data);

  std::span<std::byte> result_view{result};

  for (std::size_t i = 0; i < N; ++i) {
    auto el_view = result_view.subspan(i * sizeof(T), sizeof(T));
    std::ranges::reverse(el_view);
  }

  return result;
}

}   // namespace hstd
