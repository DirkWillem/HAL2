module;

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

export module modbus.server.spec;

export import :array;
export import :enumerate;
export import :bit;

import hstd;

namespace modbus::server::spec {

/**
 * Returns the size of a given type in 16-bit registers
 * @tparam T Type to return the data size of
 * @return Size of the data type, in 16-bit registers
 */
export template <typename T>
consteval uint16_t DataSize() noexcept {
  return static_cast<uint16_t>(sizeof(T) / 2);
}

/**
 * Options for overriding the specification of a register
 * @tparam AEN Array element mapping type
 * @tparam ED Enum definition type
 */
template <typename AEN = DefaultArrayElementNaming, typename ED = hstd::Empty>
struct RegisterOpts {
  AEN array_element_naming = {};   //!< Array element naming convention
  ED  enum_def             = {};   //!< Enum definition
};

template <uint16_t A, typename D, hstd::StaticString N, RegisterOpts Opts>
struct Register {
  using Data = D;

  static constexpr auto Options      = Opts;
  static constexpr auto Name         = N;
  static constexpr auto Size         = DataSize<D>();
  static constexpr auto StartAddress = A;
  static constexpr auto EndAddress   = A + Size;
};

/**
 * Describes an input register
 * @tparam A Register address
 * @tparam D Register contained data type
 * @tparam N Register name
 * @tparam Opts Register options
 */
export template <uint16_t A, typename D, hstd::StaticString N,
                 RegisterOpts Opts = {}>
struct InputRegister : Register<A, D, N, Opts> {};

/**
 * Describes a holding register
 * @tparam A Register address
 * @tparam D Register contained data type
 * @tparam N Register name
 * @tparam Opts Register options
 */
export template <uint16_t A, typename D, hstd::StaticString N,
                 RegisterOpts Opts = {}>
struct HoldingRegister : Register<A, D, N, Opts> {};

namespace concepts {

template <typename T>
inline constexpr auto IsInputRegister = false;

template <uint16_t A, typename D, hstd::StaticString N, RegisterOpts Opts>
inline constexpr auto IsInputRegister<InputRegister<A, D, N, Opts>> = true;

/**
 * Concept for determining if a type is an input register description
 */
export template <typename T>
concept InputRegister = IsInputRegister<T>;

export template <typename T, typename S>
concept InputRegisterEntry = requires {
  requires InputRegister<S>;
  requires std::is_same_v<typename std::remove_cvref_t<T>::Specification, S>;
};

template <typename T>
inline constexpr auto IsHoldingRegister = false;

template <uint16_t A, typename D, hstd::StaticString N, RegisterOpts Opts>
inline constexpr auto IsHoldingRegister<HoldingRegister<A, D, N, Opts>> = true;

/**
 * Concept for determining if a type is a holding register description
 */
export template <typename T>
concept HoldingRegister = IsHoldingRegister<T>;

export template <typename T, typename S>
concept HoldingRegisterEntry = requires {
  requires HoldingRegister<S>;
  requires std::is_same_v<typename std::remove_cvref_t<T>::Specification, S>;
};

/**
 * Concept for determining if a type is a register (input or holding)
 * description
 */
export template <typename T>
concept Register = InputRegister<T> || HoldingRegister<T>;

}   // namespace concepts

/**
 * Specification of a MODBUS server
 * @tparam DIs Discrete input list
 * @tparam Cs Coils list
 * @tparam IRs Input registers list
 * @tparam HRs Holding registers list
 */
export template <hstd::concepts::Types DIs, hstd::concepts::Types Cs,
                 hstd::concepts::Types IRs, hstd::concepts::Types HRs>
struct ServerSpec {
  using DiscreteInputs   = DIs;   //!< Discrete input specs
  using Coils            = Cs;    //!< Coil specs
  using InputRegisters   = IRs;   //!< Input register specs
  using HoldingRegisters = HRs;   //!< Holding register specs
};

namespace concepts {

template <typename>
inline constexpr bool IsServerSpec = false;

template <typename DIs, typename Cs, typename IRs, typename HRs>
inline constexpr bool IsServerSpec<ServerSpec<DIs, Cs, IRs, HRs>> = true;

/**
 * Concept that determines if a type is a valid server specification
 * @tparam T Checked type
 */
export template <typename T>
concept ServerSpec = IsServerSpec<T>;

}   // namespace concepts

}   // namespace modbus::server::spec