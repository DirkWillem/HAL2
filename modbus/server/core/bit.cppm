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

/** Concept describing storage for a mutable bit */
export template <typename T>
concept MutableBitStorage = std::is_same_v<T, uint8_t>;

/** Concept describing storage for a read-only bit */
export template <typename T>
concept ReadonlyBitStorage = MutableBitStorage<T>;

/**
 * Single discrete input
 * @tparam Spec Discrete input specification
 * @tparam S Storage type
 */
export template <spec::concepts::DiscreteInput Spec, ReadonlyBitStorage S>
struct DiscreteInput {
  using Specification = Spec;   //!< Specification type
  using Storage       = S;      //!< Storage type
  using Bits          = S;      //!< Type able to represent the bits in memory

  static constexpr auto Address = Spec::StartAddress;   //!< Bit start address
};

/**
 * Whether a type is a discrete input
 * @tparam T Type to check
 */
export template <typename T>
inline constexpr bool IsDiscreteInput = false;

export template <typename Spec, MutableBitStorage S>
inline constexpr bool IsDiscreteInput<DiscreteInput<Spec, S>> = true;

/**
 * Type alias for an in-memory discrete input
 * @tparam Spec Discrete input specification
 */
export template <spec::concepts::Bit Spec>
using InMemDiscreteInput = DiscreteInput<Spec, uint8_t>;

/**
 * Single coil
 * @tparam Spec Discrete input specification
 * @tparam S Storage type
 */
export template <spec::concepts::Coil Spec, MutableBitStorage S>
struct Coil {
  using Specification = Spec;   //!< Specification type
  using Storage       = S;      //!< Storage type
  using Bits          = S;      //!< Type able to represent the bits in memory

  static constexpr auto Address = Spec::StartAddress;   //!< Bit start address
};

/**
 * Whether a type is a coil
 * @tparam T Type to check
 */
export template <typename T>
inline constexpr bool IsCoil = false;

export template <typename Spec, MutableBitStorage S>
inline constexpr bool IsCoil<Coil<Spec, S>> = true;

/**
 * Type alias for an in-memory coil
 * @tparam Spec Discrete input specification
 */
export template <spec::concepts::Bit Spec>
using InMemCoil = Coil<Spec, uint8_t>;

/**
 * Reads a single bit from a bit storage
 * @tparam S Storage type
 * @param storage Storage to read from
 * @return Read bit
 */
export template <ReadonlyBitStorage S>
  requires(std::is_same_v<S, uint8_t>)
constexpr std::expected<uint8_t, ExceptionCode>
ReadBit(const S& storage) noexcept {
  constexpr auto Msk = S{0b1U};
  return (storage & Msk) == Msk;
}

/**
 * Writes a single bit to a bit storage
 * @tparam S Storage type
 * @param storage Storage to write to
 * @param value Value to write
 * @return Written value, or exception code on failure
 */
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

/** Concept describing a custom storage for read-only bit sets */
template <typename Impl>
concept CustomReadonlyBitSetStorage = requires(const Impl& c_impl) {
  { Impl::MaxBitCount } -> std::convertible_to<std::size_t>;
  // Storage type should have an in-memory representation type
  requires std::unsigned_integral<typename Impl::MemType>;

  // In-memory representation type should be able to contain all bits
  requires(Impl::MaxBitCount
           <= std::numeric_limits<typename Impl::MemType>::digits);

  // Storage should have a const Read method
  {
    c_impl.Read(std::declval<typename Impl::MemType>())
  }
  -> std::convertible_to<std::expected<typename Impl::MemType, ExceptionCode>>;
};

/** Concept describing a custom storage for mutable bit sets */
template <typename Impl>
concept CustomMutableBitSetStorage = requires(Impl& impl) {
  // Storage should have a Write method
  {
    impl.Write(std::declval<typename Impl::MemType>(),
               std::declval<typename Impl::MemType>())
  }
  -> std::convertible_to<std::expected<typename Impl::MemType, ExceptionCode>>;
};

/**
 * Concept describing storage for a mutable bit set
 * @tparam T Implementation type
 * @tparam C Bit count
 */
export template <typename T, uint16_t C>
concept MutableBitSetStorage =
    (std::is_unsigned_v<T> && C <= std::numeric_limits<T>::digits)
    || (CustomMutableBitSetStorage<T> && C <= T::MaxBitCount);

/**
 * Concept describing storage for a read-only bit set
 * @tparam T Implementation type
 * @tparam C Bit count
 */
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

/**
 * Set of discrete inputs
 * @tparam Spec Discrete inputs specification
 * @tparam S Storage type
 */
export template <spec::concepts::DiscreteInputs       Spec,
                 ReadonlyBitSetStorage<(Spec::Count)> S>
struct DiscreteInputSet {
  using Specification = Spec;                         //!< Specification type
  using Storage       = S;                            //!< Storage type
  using Bits = typename UnderlyingBitType<S>::Type;   //!< In-mem representation

  static constexpr auto StartAddress = Spec::StartAddress;   //!< Start address
  static constexpr auto EndAddress   = Spec::EndAddress;     //!< End address
  static constexpr auto Count        = Spec::Count;          //!< Number of bits

  static_assert(Spec::StartAddress % 8 == 0,
                "Discrete input set starting addresses must be byte-aligned");
};

/**
 * Whether a type is a discrete input set
 * @tparam T Type to check
 */
export template <typename T>
inline constexpr bool IsDiscreteInputSet = false;

export template <spec::concepts::Bits                 Spec,
                 ReadonlyBitSetStorage<(Spec::Count)> S>
inline constexpr bool IsDiscreteInputSet<DiscreteInputSet<Spec, S>> = true;

/**
 * Type alias for an in-memory discrete input set
 * @tparam Spec Discrete inputs specification
 */
export template <spec::concepts::Bits Spec>
  requires(Spec::Count <= 32)
using InMemDiscreteInputSet =
    DiscreteInputSet<Spec, hstd::UintN_t<(Spec::Count)>>;

/**
 * Set of coils
 * @tparam Spec Discrete inputs specification
 * @tparam S Storage type
 */
export template <spec::concepts::Coils               Spec,
                 MutableBitSetStorage<(Spec::Count)> S>
struct CoilSet {
  using Specification = Spec;                         //!< Specification type
  using Storage       = S;                            //!< Storage type
  using Bits = typename UnderlyingBitType<S>::Type;   //!< In-mem representation

  static constexpr auto StartAddress = Spec::StartAddress;   //!< Start address
  static constexpr auto EndAddress   = Spec::EndAddress;     //!< End address
  static constexpr auto Count        = Spec::Count;          //!< Number of bits

  static_assert(Spec::StartAddress % 8 == 0,
                "Discrete input set starting addresses must be byte-aligned");
};

/**
 * Whether a type is a coil input set
 * @tparam T Type to check
 */
export template <typename T>
inline constexpr bool IsCoilSet = false;

export template <spec::concepts::Coils               Spec,
                 MutableBitSetStorage<(Spec::Count)> S>
inline constexpr bool IsCoilSet<CoilSet<Spec, S>> = true;

/**
 * Type alias for an in-memory coil input set
 * @tparam Spec Discrete inputs specification
 */
export template <spec::concepts::Bits Spec>
  requires(Spec::Count <= 32)
using InMemCoilSet = CoilSet<Spec, hstd::UintN_t<(Spec::Count)>>;

/**
 * Reads a set of bits from a bit storage, implementation for in-memory storage
 * @tparam S Storage type
 * @param storage Storage to read from
 * @param mask Mask of the bits to read
 * @return Read bits, or exception code on failure
 */
export template <typename S>
  requires(std::is_unsigned_v<S>)
constexpr std::expected<S, ExceptionCode> ReadBitSet(const S& storage,
                                                     S        mask) noexcept {
  return {storage & mask};
}

/**
 * Reads a set of bits from a bit storage, implementation for custom storage
 * @tparam S Storage type
 * @param storage Storage to read from
 * @param mask Mask of the bits to read
 * @return Read bits, or exception code on failure
 */
export template <CustomReadonlyBitSetStorage S>
constexpr std::expected<typename S::MemType, ExceptionCode>
ReadBitSet(S& storage, typename S::MemType mask) {
  return storage.Read(mask);
}

/**
 * Writes bits to a bit storage, implementation for in-memory storage
 * @tparam S Storage type
 * @param storage Storage to write to
 * @param mask Mask of the bits to write
 * @param value Bits to write
 * @return Written bits, or exception code on failure
 */
export template <typename S>
  requires(std::is_unsigned_v<S>)
constexpr std::expected<S, ExceptionCode> WriteBitSet(S& storage, S mask,
                                                      S value) noexcept {
  storage = (storage & ~mask) | (value & mask);
  return {value & mask};
}

/**
 * Writes bits to a bit storage, implementation for custom storage
 * @tparam S Storage type
 * @param storage Storage to write to
 * @param mask Mask of the bits to write
 * @param value Bits to write
 * @return Written bits, or exception code on failure
 */
export template <CustomMutableBitSetStorage S>
constexpr std::expected<typename S::MemType, ExceptionCode>
WriteBitSet(S& storage, typename S::MemType mask, typename S::MemType value) {
  return storage.Write(mask, value);
}

namespace concepts {

/** Concept describing a single bit (either discrete input or coil) */
export template <typename T>
concept SingleBit = IsDiscreteInput<T> || IsCoil<T>;

/** Concept describing a set of bits (either discrete inputs or coils) */
export template <typename T>
concept BitSet = IsDiscreteInputSet<T> || IsCoilSet<T>;

/** Concept describing one or more discrete inputs */
export template <typename T>
concept DiscreteInput = IsDiscreteInput<T> || IsDiscreteInputSet<T>;

/** Concept describing one or more coils */
export template <typename T>
concept Coil = IsCoil<T> || IsCoilSet<T>;

/** Concept describing one or more bits */
export template <typename T>
concept Bits = DiscreteInput<T> || Coil<T>;

}   // namespace concepts

}   // namespace modbus::server