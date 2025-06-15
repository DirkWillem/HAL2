module;

#include <concepts>
#include <cstdint>
#include <limits>

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
export template <std::unsigned_integral T>
constexpr T Ones(std::size_t n) noexcept {
  if (n >= std::numeric_limits<T>::digits) {
    return std::numeric_limits<T>::max();
  }

  return (T{0b1U} << n) - 1;
}

}   // namespace hstd