module;

#include <array>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <utility>

export module modbus.server.spec:bit;

import hstd;

namespace modbus::server::spec {

/**
 * Default naming convention for bits, appends the index to the root name
 */
export struct DefaultBitNaming {
  template <hstd::StaticString Root, std::size_t Idx>
  constexpr auto BitName() const noexcept {
    hstd::StaticStringBuilder<Root.size() + hstd::NumDigits(Idx)> result;
    result.Append(static_cast<std::string_view>(Root));
    result.Append(Idx);
    return result;
  }
};

/**
 * Naming convention for bits where an array of names is provided
 */
export template <hstd::StaticString... Names>
struct ArrayBitNaming {
  template <hstd::StaticString, std::size_t Idx>
  constexpr auto BitName() const noexcept {
    if (Idx >= sizeof...(Names)) {
      std::unreachable();
    }

    return std::get<Idx>(std::make_tuple(Names...));
  }
};

namespace concepts {

/**
 * Concept that describes a bit naming convention
 * @tparam Impl Implementing type
 */
export template <typename Impl>
concept BitNaming = requires(const Impl& c_impl) {
  {
    c_impl.template BitName<hstd::StaticString{"MyBits"}, 2>()
  } -> hstd::concepts::ToStringView;
};

}   // namespace concepts

/**
 * Allowed accesses for a bit
 */
export enum class BitAccess {
  ReadWrite,       //!< Client can read and write the bit to either 0 or 1
  ReadWrite0,      //!< Client can read the bit and write it to 0 only
  ReadWrite1,      //!< Client can read the bit and write it to 1 only
  DiscreteInput,   //!< Bit is a discrete input
};

/**
 * Options for overriding the specification of a bit
 * @tparam BN Bit naming convention type
 */
export template <typename BN = DefaultBitNaming>
struct BitOpts {
  BitAccess access     = BitAccess::ReadWrite;   //!< Access for the bit
  BN        bit_naming = {};
};

template <uint16_t A, uint16_t C, hstd::StaticString N, BitOpts Opts = {}>
struct Bits {
  static constexpr auto Options      = Opts;   //!< Options for the bit
  static constexpr auto Name         = N;      //!< Name of the bit
  static constexpr auto StartAddress = A;      //!< Start address of the bit
  static constexpr auto Count        = C;      //!< Number of bits
  static constexpr auto EndAddress   = A + Count;   //!< End address of the bits
};

/**
 * Describes a single discrete input
 * @tparam A Address of the bit
 * @tparam C Number of inputs
 * @tparam N Name of the bit
 * @tparam Opts Bit options
 */
export template <uint16_t A, uint16_t C, hstd::StaticString N,
                 BitOpts Opts = {}>
struct DiscreteInputs : Bits<A, C, N, Opts> {};

/**
 * Describes a single coil
 * @tparam A Address of the bit
 * @tparam C Number of coils
 * @tparam N Name of the bit
 * @tparam Opts Bit options
 */
export template <uint16_t A, uint16_t C, hstd::StaticString N,
                 BitOpts Opts = {}>
struct Coils : Bits<A, C, N, Opts> {};

/**
 * Describes a single discrete input
 * @tparam A Address of the bit
 * @tparam N Name of the bit
 * @tparam Opts Bit options
 */
export template <uint16_t A, hstd::StaticString N, BitOpts Opts = {}>
using DiscreteInput = DiscreteInputs<A, 1, N, Opts>;

/**
 * Describes a single coil
 * @tparam A Address of the bit
 * @tparam N Name of the bit
 * @tparam Opts Bit options
 */
export template <uint16_t A, hstd::StaticString N, BitOpts Opts = {}>
using Coil = Coils<A, 1, N, Opts>;

namespace concepts {
template <typename T>
inline constexpr auto IsDiscreteInputs = false;

template <uint16_t A, uint16_t C, hstd::StaticString N, BitOpts Opts>
inline constexpr auto IsDiscreteInputs<DiscreteInputs<A, C, N, Opts>> = true;

/**
 * Concept for determining if a type is a discrete input description
 */
export template <typename T>
concept DiscreteInputs = IsDiscreteInputs<T>;

export template <typename T, typename S>
concept DiscreteInputsEntry = requires {
  requires DiscreteInputs<S>;
  requires std::is_same_v<typename std::remove_cvref_t<T>::Specification, S>;
};

export template <typename T>
concept DiscreteInput = DiscreteInputs<T> && (T::Count == 1);

export template <typename T, typename S>
concept DiscreteInputEntry = requires {
  requires DiscreteInput<S>;
  requires std::is_same_v<typename std::remove_cvref_t<T>::Specification, S>;
};

template <typename T>
inline constexpr auto IsCoils = false;

template <uint16_t A, uint16_t C, hstd::StaticString N, BitOpts Opts>
inline constexpr auto IsCoils<Coils<A, C, N, Opts>> = true;

/**
 * Concept for determining if a type is a coil description
 */
export template <typename T>
concept Coils = IsCoils<T>;

export template <typename T, typename S>
concept CoilsEntry = requires {
  requires Coils<S>;
  requires std::is_same_v<typename std::remove_cvref_t<T>::Specification, S>;
};

export template <typename T>
concept Coil = Coils<T> && (T::Count == 1);

export template <typename T, typename S>
concept CoilEntry = requires {
  requires Coil<S>;
  requires std::is_same_v<typename std::remove_cvref_t<T>::Specification, S>;
};

export template <typename T>
concept Bits = DiscreteInputs<T> || Coils<T>;

export template <typename T>
concept Bit = Bits<T> && (T::Count == 1);

}   // namespace concepts

}   // namespace modbus::server::spec