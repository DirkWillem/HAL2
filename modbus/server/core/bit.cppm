module;

#include <cstdint>
#include <expected>
#include <limits>
#include <string_view>

export module modbus.server:bit;

import hstd;

import modbus.core;
import modbus.server.spec;

namespace modbus::server {
export template <typename T>
concept MutableBitStorage = std::is_same_v<T, uint8_t>;

export template <typename T>
concept ReadonlyBitStorage = MutableBitStorage<T>;

export template <spec::concepts::DiscreteInput Spec, ReadonlyBitStorage S>
struct DiscreteInput {
  using Storage = S;
  using Bits    = S;

  static constexpr auto Address = Spec::StartAddress;
};

export template <typename T>
inline constexpr bool IsDiscreteInput = false;

export template <typename Spec, MutableBitStorage S>
inline constexpr bool IsDiscreteInput<DiscreteInput<Spec, S>> = true;

export template <spec::concepts::Bit Spec>
using InMemDiscreteInput = DiscreteInput<Spec, uint8_t>;

export template <spec::concepts::Coil Spec, MutableBitStorage S>
struct Coil {
  using Storage = S;
  using Bits    = S;

  static constexpr auto Address = Spec::StartAddress;
};

export template <typename T>
inline constexpr bool IsCoil = false;

export template <typename Spec, MutableBitStorage S>
inline constexpr bool IsCoil<Coil<Spec, S>> = true;

export template <spec::concepts::Bit Spec>
using InMemCoil = Coil<Spec, uint8_t>;

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

template <typename T>
struct UnderlyingBitType;

template <std::unsigned_integral T>
struct UnderlyingBitType<T> {
  using Type = T;
};

template <CustomReadonlyBitSetStorage S>
struct UnderlyingBitType<S> {
  using Type = typename S::MemType;
};

export template <spec::concepts::DiscreteInputs       Spec,
                 ReadonlyBitSetStorage<(Spec::Count)> S>
struct DiscreteInputSet {
  using Storage = S;
  using Bits    = typename UnderlyingBitType<S>::Type;

  static constexpr auto StartAddress = Spec::StartAddress;
  static constexpr auto EndAddress   = Spec::EndAddress;
  static constexpr auto Count        = Spec::Count;

  static_assert(Spec::StartAddress % 8 == 0,
                "Discrete input set starting addresses must be byte-aligned");
};

export template <typename T>
inline constexpr bool IsDiscreteInputSet = false;

export template <spec::concepts::Bits                 Spec,
                 ReadonlyBitSetStorage<(Spec::Count)> S>
inline constexpr bool IsDiscreteInputSet<DiscreteInputSet<Spec, S>> = true;

export template <spec::concepts::Bits Spec>
  requires(Spec::Count <= 32)
using InMemDiscreteInputSet =
    DiscreteInputSet<Spec, hstd::UintN_t<(Spec::Count)>>;

export template <spec::concepts::Coils               Spec,
                 MutableBitSetStorage<(Spec::Count)> S>
struct CoilSet {
  using Storage = S;
  using Bits    = typename UnderlyingBitType<S>::Type;

  static constexpr auto StartAddress = Spec::StartAddress;
  static constexpr auto EndAddress   = Spec::EndAddress;
  static constexpr auto Count        = Spec::Count;

  static_assert(Spec::StartAddress % 8 == 0,
                "Coil set starting addresses must be byte-aligned");
};

export template <typename T>
inline constexpr bool IsCoilSet = false;

export template <spec::concepts::Coils               Spec,
                 MutableBitSetStorage<(Spec::Count)> S>
inline constexpr bool IsCoilSet<CoilSet<Spec, S>> = true;

export template <spec::concepts::Bits Spec>
  requires(Spec::Count <= 32)
using InMemCoilSet = CoilSet<Spec, hstd::UintN_t<(Spec::Count)>>;

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