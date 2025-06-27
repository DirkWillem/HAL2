module;

#include <concepts>
#include <cstdint>
#include <limits>
#include <memory>

export module hstd:math;

namespace hstd {

export [[nodiscard]] constexpr bool
IsPowerOf2(std::unsigned_integral auto v) noexcept {
  return v && ((v & (v - 1)) == 0);
}

export template <bool Signed, unsigned Bits>
struct IntN;

export template <bool Signed, unsigned Bits>
  requires(Bits <= 8)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int8_t, uint8_t>;
};

export template <bool Signed, unsigned Bits>
  requires(Bits > 8 && Bits <= 16)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int16_t, uint16_t>;
};

export template <bool Signed, unsigned Bits>
  requires(Bits > 16 && Bits <= 32)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int32_t, uint32_t>;
};

export template <bool Signed, unsigned Bits>
  requires(Bits > 32 && Bits <= 64)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int64_t, uint64_t>;
};

export template <bool Signed, unsigned Bits>
using IntN_t = IntN<Signed, Bits>::T;

export template <unsigned Bits>
using UintN_t = IntN_t<false, Bits>;

/**
 * Returns a given amount of 1 bits
 * @tparam T Unsigned integral type
 * @param n Number of ones to return
 * @return n ones
 */
export template <typename T>
  requires std::unsigned_integral<T> || std::is_same_v<T, std::byte>
constexpr T Ones(std::size_t n) noexcept {
  if constexpr (std::is_same_v<T, std::byte>) {
    return std::byte{Ones<uint8_t>(n)};
  } else {
    if (n >= std::numeric_limits<T>::digits) {
      return std::numeric_limits<T>::max();
    }

    return (T{0b1U} << n) - 1;
  }
}

namespace literals {

export constexpr uint8_t operator""_u8(unsigned long long int x) noexcept {
  return static_cast<uint8_t>(x);
}

export constexpr std::byte operator""_b(unsigned long long int x) noexcept {
  return static_cast<std::byte>(x);
}

export constexpr uint16_t operator""_u16(unsigned long long int x) noexcept {
  return static_cast<uint16_t>(x);
}

export constexpr uint32_t operator""_u32(unsigned long long int x) noexcept {
  return static_cast<uint32_t>(x);
}



}

}   // namespace hstd