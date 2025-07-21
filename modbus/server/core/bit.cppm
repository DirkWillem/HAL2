module;

#include <cstdint>
#include <expected>
#include <limits>
#include <string_view>

export module modbus.server:bit;

import hstd;

import modbus.core;

namespace modbus::server {
export template <typename T>
concept MutableBitStorage = std::is_same_v<T, uint8_t>;

export template <typename T>
concept ReadonlyBitStorage = MutableBitStorage<T>;

export template <uint16_t A, ReadonlyBitStorage S, hstd::StaticString N>
struct DiscreteInput {
  using Storage = S;
  using Bits    = S;

  static constexpr auto             Address = A;
  static constexpr std::string_view Name    = N;
};

export template <typename T>
inline constexpr bool IsDiscreteInput = false;

export template <uint16_t A, MutableBitStorage S, hstd::StaticString N>
inline constexpr bool IsDiscreteInput<DiscreteInput<A, S, N>> = true;

export template <uint16_t A, hstd::StaticString N>
using InMemDiscreteInput = DiscreteInput<A, uint8_t, N>;

export template <uint16_t A, MutableBitStorage S, hstd::StaticString N>
struct Coil {
  using Storage = S;
  using Bits    = S;

  static constexpr auto             Address = A;
  static constexpr std::string_view Name    = N;
};

export template <typename T>
inline constexpr bool IsCoil = false;

export template <uint16_t A, MutableBitStorage S, hstd::StaticString N>
inline constexpr bool IsCoil<Coil<A, S, N>> = true;

export template <uint16_t A, hstd::StaticString N>
using InMemCoil = Coil<A, uint8_t, N>;

export template <ReadonlyBitStorage S>
  requires(std::is_same_v<S, uint8_t>)
constexpr std::expected<uint8_t, ExceptionCode>
ReadBit(const S& storage) noexcept {
  constexpr auto Msk = S{0b1U};
  return (storage & Msk) == Msk;
}

export template <MutableBitStorage S>
  requires(std::is_same_v<S, uint8_t>)
constexpr std::expected<uint8_t, ExceptionCode>
WriteBit(S& storage, uint8_t value) noexcept {
  constexpr auto Msk = S{0b1U};

  if (value) {
    storage |= Msk;
  } else {
    storage &= ~Msk;
  }

  return {value};
}

template <typename Impl>
concept CustomReadonlyBitSetStorage = requires(const Impl& c_impl) {
  { Impl::MaxBitCount } -> std::convertible_to<std::size_t>;
  requires std::unsigned_integral<typename Impl::MemType>;
  requires(Impl::MaxBitCount
           <= std::numeric_limits<typename Impl::MemType>::digits);

  {
    c_impl.Read(std::declval<typename Impl::MemType>())
  }
  -> std::convertible_to<std::expected<typename Impl::MemType, ExceptionCode>>;
};

template <typename Impl>
concept CustomMutableBitSetStorage = requires(Impl& impl) {
  {
    impl.Write(std::declval<typename Impl::MemType>(),
               std::declval<typename Impl::MemType>())
  }
  -> std::convertible_to<std::expected<typename Impl::MemType, ExceptionCode>>;
};

export template <typename T, uint16_t C>
concept MutableBitSetStorage =
    (std::is_unsigned_v<T> && C <= std::numeric_limits<T>::digits)
    || (CustomMutableBitSetStorage<T> && C <= T::MaxBitCount);

export template <typename T, uint16_t C>
concept ReadonlyBitSetStorage =
    MutableBitSetStorage<T, C>
    || (CustomReadonlyBitSetStorage<T> && C <= T::MaxBitCount);

template<typename T>
struct UnderlyingBitType;

template<std::unsigned_integral T>
struct UnderlyingBitType<T> {
  using Type = T;
};

template<CustomReadonlyBitSetStorage S>
struct UnderlyingBitType<S> {
  using Type = typename S::MemType;
};

export template <uint16_t A, uint16_t C, ReadonlyBitSetStorage<C> S,
                 hstd::StaticString N>
struct DiscreteInputSet {
  using Storage = S;
  using Bits    = UnderlyingBitType<S>::Type;

  static constexpr auto             StartAddress = A;
  static constexpr auto             EndAddress   = A + C;
  static constexpr auto             Count        = C;
  static constexpr std::string_view Name         = N;

  static_assert(A % 8 == 0,
                "Discrete input set starting addresses must be byte-aligned");
};

export template <typename T>
inline constexpr bool IsDiscreteInputSet = false;

export template <uint16_t A, uint16_t C, MutableBitSetStorage<C> S,
                 hstd::StaticString N>
inline constexpr bool IsDiscreteInputSet<DiscreteInputSet<A, C, S, N>> = true;
export template <uint16_t A, uint16_t C, hstd::StaticString N>
  requires(C <= 32)
using InMemDiscreteInputSet = DiscreteInputSet<A, C, hstd::UintN_t<C>, N>;

export template <uint16_t A, uint16_t C, MutableBitSetStorage<C> S,
                 hstd::StaticString N>
struct CoilSet {
  using Storage = S;
  using Bits    = UnderlyingBitType<S>::Type;

  static constexpr auto             StartAddress = A;
  static constexpr auto             EndAddress   = A + C;
  static constexpr auto             Count        = C;
  static constexpr std::string_view Name         = N;

  static_assert(A % 8 == 0, "Coil set starting addresses must be byte-aligned");
};

export template <typename T>
inline constexpr bool IsCoilSet = false;

export template <uint16_t A, uint16_t C, MutableBitSetStorage<C> S,
                 hstd::StaticString N>
inline constexpr bool IsCoilSet<CoilSet<A, C, S, N>> = true;

export template <uint16_t A, uint16_t C, hstd::StaticString N>
  requires(C <= 32)
using InMemCoilSet = CoilSet<A, C, hstd::UintN_t<C>, N>;

export template <typename S>
  requires(std::is_unsigned_v<S>)
constexpr std::expected<S, ExceptionCode> ReadBitSet(const S& storage,
                                                     S        mask) noexcept {
  return {storage & mask};
}

export template <CustomReadonlyBitSetStorage S>
constexpr std::expected<typename S::MemType, ExceptionCode>
ReadBitSet(S& storage, typename S::MemType mask) {
  return storage.Read(mask);
}

export template <typename S>
  requires(std::is_unsigned_v<S>)
constexpr std::expected<S, ExceptionCode> WriteBitSet(S& storage, S mask,
                                                      S value) noexcept {
  storage = (storage & ~mask) | (value & mask);
  return {value & mask};
}

export template <CustomMutableBitSetStorage S>
constexpr std::expected<typename S::MemType, ExceptionCode>
WriteBitSet(S& storage, typename S::MemType mask, typename S::MemType value) {
  return storage.Write(mask, value);
}

namespace concepts {

export template <typename T>
concept SingleBit = IsDiscreteInput<T> || IsCoil<T>;

export template <typename T>
concept BitSet = IsDiscreteInputSet<T> || IsCoilSet<T>;

export template <typename T>
concept DiscreteInput = IsDiscreteInput<T> || IsDiscreteInputSet<T>;

export template <typename T>
concept Coil = IsCoil<T> || IsCoilSet<T>;

export template <typename T>
concept Bits = DiscreteInput<T> || Coil<T>;

}   // namespace concepts

}   // namespace modbus::server