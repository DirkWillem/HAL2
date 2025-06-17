module;

#include <cstdint>
#include <expected>
#include <limits>
#include <string_view>

export module modbus.server:coil;

import hstd;

import modbus.core;

namespace modbus::server {

export template <typename T>
concept CoilStorage = std::is_same_v<T, uint8_t>;

export template <uint16_t A, CoilStorage S, hstd::StaticString N>
struct Coil {
  using Storage = S;
  using Bits    = S;

  static constexpr auto             Address = A;
  static constexpr std::string_view Name    = N;
};

export template <uint16_t A, hstd::StaticString N>
using InMemCoil = Coil<A, uint8_t, N>;

export template <CoilStorage S>
  requires(std::is_same_v<S, uint8_t>)
constexpr std::expected<uint8_t, ExceptionCode>
ReadCoil(const S& storage) noexcept {
  constexpr auto Msk = S{0b1U};
  return (storage & Msk) == Msk;
}

export template <CoilStorage S>
  requires(std::is_same_v<S, uint8_t>)
constexpr std::expected<uint8_t, ExceptionCode>
WriteCoil(S& storage, uint8_t value) noexcept {
  constexpr auto Msk = S{0b1U};

  if (value) {
    storage |= Msk;
  } else {
    storage &= ~Msk;
  }

  return {value};
}

export template <typename T>
inline constexpr bool IsCoil = false;

export template <uint16_t A, CoilStorage S, hstd::StaticString N>
inline constexpr bool IsCoil<Coil<A, S, N>> = true;

export template <typename T, uint16_t C>
concept CoilSetStorage =
    (std::is_unsigned_v<T> && C <= std::numeric_limits<T>::digits);

export template <uint16_t A, uint16_t C, CoilSetStorage<C> S,
                 hstd::StaticString N>
struct CoilSet {
  using Storage = S;
  using Bits    = S;

  static constexpr auto             StartAddress = A;
  static constexpr auto             EndAddress   = A + C;
  static constexpr auto             Count        = C;
  static constexpr std::string_view Name         = N;

  static_assert(A % 8 == 0, "Coil set starting addresses must be byte-aligned");
};

export template <uint16_t A, uint16_t C, hstd::StaticString N>
  requires(C <= 32)
using InMemCoilSet = CoilSet<A, C, hstd::UintN_t<C>, N>;

export template <typename S>
  requires(std::is_unsigned_v<S>)
constexpr std::expected<S, ExceptionCode> ReadCoilSet(const S& storage,
                                                      S        mask) noexcept {
  return {storage & mask};
}

export template <typename S>
  requires(std::is_unsigned_v<S>)
constexpr std::expected<S, ExceptionCode> WriteCoilSet(S& storage, S mask,
                                                       S value) noexcept {
  storage = (storage & ~mask) | (value & mask);
  return {value & mask};
}

export template <typename T>
inline constexpr bool IsCoilSet = false;

export template <typename T>
concept CoilOrCoilSet = IsCoil<T> || IsCoilSet<T>;

export template <uint16_t A, uint16_t C, CoilSetStorage<C> S,
                 hstd::StaticString N>
inline constexpr bool IsCoilSet<CoilSet<A, C, S, N>> = true;

}   // namespace modbus::server