module;

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <memory>
#include <ranges>
#include <span>

export module hstd:memory;

import :endian;

namespace hstd {

export template <std::endian E = std::endian::native, typename T>
constexpr auto ToByteArray(const T& data) noexcept {
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(
      ConvertToEndianness<E>(data));
}

export template <std::endian E = std::endian::native, typename T, std::size_t N>
  requires(E != std::endian::native)
constexpr auto ToByteArray(const std::array<T, N>& data) noexcept {
  auto result = std::bit_cast<std::array<std::byte, sizeof(T) * N>>(data);

  std::span<std::byte> result_view{result};

  for (std::size_t i = 0; i < N; ++i) {
    auto el_view = result_view.subspan(i * sizeof(T), sizeof(T));
    std::ranges::reverse(el_view);
  }

  return result;
}

export template <std::endian E = std::endian::native, typename T, std::size_t N>
  requires(E == std::endian::native)
constexpr auto ToByteArray(const std::array<T, N>& data) noexcept {
  return std::bit_cast<std::array<std::byte, sizeof(T) * N>>(data);
}

export template <std::endian E = std::endian::native, typename T>
  requires(E == std::endian::native)
void IntoByteArray(std::span<std::byte> into, T value) noexcept {
  if (E != std::endian::native) {
    value = SwapEndianness(value);
  }

  std::memcpy(into.data(), &value, sizeof(T));
}

export template <std::integral T, std::endian E = std::endian::native>
constexpr auto BytesToInt(std::span<const std::byte> bytes) {
  T result;
  std::memcpy(&result, bytes.data(), sizeof(T));

  if (E != std::endian::native) {
    return SwapEndianness(result);
  } else {
    return result;
  }
}

export template <std::floating_point T, std::endian E = std::endian::native>
constexpr auto BytesToFloat(std::span<const std::byte> bytes) {
  T result;
  std::memcpy(&result, bytes.data(), sizeof(T));

  if (E != std::endian::native) {
    return SwapEndianness(result);
  } else {
    return result;
  }
}

}   // namespace hstd
