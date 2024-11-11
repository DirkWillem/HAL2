#pragma once

#include <cstdint>

namespace ct {

template <bool Signed, unsigned Bits>
struct IntN;

template <bool Signed, unsigned Bits>
  requires(Bits <= 8)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int8_t, uint8_t>;
};

template <bool Signed, unsigned Bits>
  requires(Bits > 8 && Bits <= 16)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int16_t, uint16_t>;
};

template <bool Signed, unsigned Bits>
  requires(Bits > 16 && Bits <= 32)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int32_t, uint32_t>;
};

template <bool Signed, unsigned Bits>
  requires(Bits > 32 && Bits <= 64)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int64_t, uint64_t>;
};

template <bool Signed, unsigned Bits>
using IntN_t = IntN<Signed, Bits>::T;

template <unsigned Bits>
using UintN_t = IntN_t<false, Bits>;

}   // namespace ct