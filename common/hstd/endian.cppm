module;

#include <bit>
#include <cstdint>

export module hstd:endian;

namespace hstd {

constexpr uint8_t SwapEndianness(uint8_t v) noexcept {
  return v;
}
constexpr uint16_t SwapEndianness(uint16_t v) noexcept {
  return (v << 8U) | (v >> 8U);
}
constexpr uint32_t SwapEndianness(uint32_t v) noexcept {
  const uint32_t tmp = ((v << 8U) & 0xFF00'FF00) | ((v >> 8U) & 0x00FF'00FF);
  return (tmp << 16U) | (tmp >> 16U);
}

constexpr float SwapEndianness(float v) noexcept {
  return std::bit_cast<float>(SwapEndianness(std::bit_cast<uint32_t>(v)));
}

export template <std::endian To>
  requires(To == std::endian::native)
constexpr auto ConvertToEndianness(auto in) noexcept {
  return in;
}

export template <std::endian To>
  requires(To != std::endian::native)
constexpr auto ConvertToEndianness(std::unsigned_integral auto in) noexcept {
  return SwapEndianness(in);
}

export template <std::endian To>
  requires(To != std::endian::native)
constexpr auto ConvertToEndianness(float in) noexcept {
  return SwapEndianness(in);
}

export template <std::endian To>
  requires(To == std::endian::native)
constexpr auto ConvertFromEndianness(auto in) noexcept {
  return in;
}

export template <std::endian To>
  requires(To != std::endian::native)
constexpr auto ConvertFromEndianness(std::unsigned_integral auto in) noexcept {
  return SwapEndianness(in);
}

export template <std::endian To>
  requires(To != std::endian::native)
constexpr auto ConvertFromEndianness(float in) noexcept {
  return SwapEndianness(in);
}

}   // namespace hstd