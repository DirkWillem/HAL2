module;

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <expected>
#include <ranges>
#include <span>
#include <utility>

export module modbus.server:reg;

import hstd;

import modbus.core;
import modbus.server.spec;

namespace modbus::server {

/** Concept for a custom read-only register storage */
template <typename Impl>
concept CustomReadonlyRegisterStorage = requires(const Impl& impl) {
  { Impl::Size } -> std::convertible_to<std::size_t>;

  {
    impl.Read(std::declval<std::size_t>(), std::declval<std::size_t>())
  } -> std::convertible_to<
      std::expected<std::span<const std::byte>, ExceptionCode>>;

  Impl::SwapEndianness(std::declval<std::span<std::byte>>(),
                       std::declval<std::size_t>(),
                       std::declval<std::size_t>());
};

/** Concept for a custom mutable register storage */
template <typename Impl>
concept CustomMutableRegisterStorage =
    CustomReadonlyRegisterStorage<Impl> && requires(Impl& impl) {
      {
        impl.Write(std::declval<std::span<const std::byte>>(),
                   std::declval<std::size_t>(), std::declval<std::size_t>())
      } -> std::convertible_to<std::expected<bool, ExceptionCode>>;
    };

/**
 * Concept for a plain memory holding register storage
 * @tparam T Checked type
 * @tparam D Data type
 */
export template <typename T, typename D>
concept PlainMemoryRegisterStorage =
    (std::same_as<T, D> && std::is_trivially_constructible_v<D>
     && std::is_trivially_copyable_v<D> && sizeof(T) % sizeof(uint16_t) == 0);

/**
 * Concept for the mutable storage of a register
 * @tparam T Holding Register type
 * @tparam D Type of the stored data
 */
export template <typename T, typename D>
concept MutableRegisterStorage =
    PlainMemoryRegisterStorage<T, D> || CustomMutableRegisterStorage<T>;

/**
 * Concept for the read-only storage of a register
 * @tparam T Holding Register type
 * @tparam D Type of the stored data
 */
export template <typename T, typename D>
concept ReadonlyRegisterStorage = MutableRegisterStorage<T, D>;

/**
 * Returns the size of a register
 * @tparam S Register storage type
 * @return Size of the register in halfwords
 */
template <typename S>
consteval std::size_t RegisterSize() noexcept {
  if constexpr (PlainMemoryRegisterStorage<S, S>) {
    return sizeof(S) / 2;
  } else if constexpr (CustomReadonlyRegisterStorage<S>) {
    return S::Size / 2;
  }

  std::unreachable();
}

/**
 * Describes an input register
 * @tparam Spec Register specification
 * @tparam S Storage implementation
 */
export template <spec::concepts::InputRegister                Spec,
                 ReadonlyRegisterStorage<typename Spec::Data> S>
struct InputRegister {
  using Data    = typename Spec::Data;   //!< Register data type
  using Storage = S;                     //!< Register storage implementation

  static constexpr auto StartAddress =
      Spec::StartAddress;   //!< Register start address
  static constexpr auto EndAddress =
      StartAddress + RegisterSize<S>();   //!< Register end address
};

/**
 * Whether a type is an input register
 * @tparam T Type to check
 */
template <typename T>
inline constexpr bool IsInputRegister = false;

template <spec::concepts::InputRegister                Spec,
          ReadonlyRegisterStorage<typename Spec::Data> S>
inline constexpr bool IsInputRegister<InputRegister<Spec, S>> = true;

/**
 * Type alias for an in-memory input register
 * @tparam Spec Register specification
 */
export template <spec::concepts::InputRegister Spec>
using InMemInputRegister = InputRegister<Spec, typename Spec::Data>;

/**
 * Describes a holding register
 * @tparam Spec Register specification
 * @tparam S Storage implementation
 */
export template <spec::concepts::HoldingRegister             Spec,
                 MutableRegisterStorage<typename Spec::Data> S>
struct HoldingRegister {
  using Data    = typename Spec::Data;   //!< Register data type
  using Storage = S;                     //!< Register storage implementation

  static constexpr auto StartAddress =
      Spec::StartAddress;   //!< Register start address
  static constexpr auto EndAddress =
      StartAddress + RegisterSize<S>();   //!< Register end address
};

/**
 * Whether a type is a holding register
 * @tparam T Type to check
 */
template <typename T>
inline constexpr bool IsHoldingRegister = false;

template <spec::concepts::HoldingRegister             Spec,
          MutableRegisterStorage<typename Spec::Data> S>
inline constexpr bool IsHoldingRegister<HoldingRegister<Spec, S>> = true;

/**
 * Type alias for an in-memory holding register
 * @tparam Spec Register specification
 */
export template <spec::concepts::HoldingRegister Spec>
using InMemHoldingRegister = HoldingRegister<Spec, typename Spec::Data>;

template <typename T>
[[nodiscard]] constexpr bool
IsAlignedRegisterAccess(std::size_t offset, std::size_t size) noexcept {
  if constexpr (hstd::IsArray<T>) {
    using ET = typename T::value_type;
    return (offset % sizeof(ET) == 0 && size % sizeof(ET) == 0);
  } else {
    return offset == 0 && size == sizeof(T);
  }
}

/**
 * Reads from a register storage, implementation for in-memory registers
 * @tparam S Storage type
 * @param storage Reference to register storage
 * @param offset Offset into the register from the register start address
 * @param size Number of bytes to read
 * @return Read bytes, or exception code on failure
 */
export template <typename S>
  requires PlainMemoryRegisterStorage<S, S>
std::expected<std::span<const std::byte>, ExceptionCode>
ReadRegister(const S& storage, std::size_t offset, std::size_t size) noexcept {
  using D = std::remove_cvref_t<S>;

  // Allow only full, aligned reads
  if (!IsAlignedRegisterAccess<D>(offset, size)) {
    return std::unexpected(ExceptionCode::ServerDeviceFailure);
  }

  // Return a byte view over the data
  return hstd::ByteViewOver<std::byte>(storage).subspan(offset, size);
}

/**
 * Reads from a register storage, implementation for custom registers
 * @tparam S Storage type
 * @param storage Reference to register storage
 * @param offset Offset into the register from the register start address
 * @param size Number of bytes to read
 * @return Read bytes, or exception code on failure
 */
export template <CustomReadonlyRegisterStorage S>
std::expected<std::span<const std::byte>, ExceptionCode>
ReadRegister(const S& storage, std::size_t offset, std::size_t size) noexcept {
  return storage.Read(offset, size);
}

/**
 * Writes to a register, implementation for in-memory registers
 * @tparam S Storage type
 * @param storage Reference to storage
 * @param data Data to write
 * @param offset Offset into the register to write at, from the register start
 * @param size Number of bytes to write
 * @return true on success, or exception code on failure
 */
export template <typename S>
  requires PlainMemoryRegisterStorage<S, S>
std::expected<bool, ExceptionCode>
WriteRegister(S& storage, std::span<const std::byte> data, std::size_t offset,
              std::size_t size) {
  using D = std::remove_cvref_t<S>;

  // Allow only full, aligned writes
  if (!IsAlignedRegisterAccess<D>(offset, size)) {
    return std::unexpected(ExceptionCode::ServerDeviceFailure);
  }

  // Write data
  std::memcpy(
      hstd::MutByteViewOver<std::byte>(storage).subspan(offset, size).data(),
      data.data(), size);
  return true;
}

/**
 * Writes to a register, implementation for custom register storage
 * @tparam S Storage type
 * @param storage Reference to storage
 * @param data Data to write
 * @param offset Offset into the register to write at, from the register start
 * @param size Number of bytes to write
 * @return true on success, or exception code on failure
 */
export template <CustomMutableRegisterStorage S>
std::expected<bool, ExceptionCode>
WriteRegister(S& storage, std::span<const std::byte> data, std::size_t offset,
              std::size_t size) {
  return storage.Write(data, offset, size);
}

/**
 * Performs an endianness swap on a byte array as if it were the raw contents of
 * a register, taking into account the register data type. Implementation for
 * in-memory register storage
 *
 * @tparam S Register storage type
 * @param data Byte view over the register data
 * @param offset Offset into the register where to perform the endianness swap
 * @param size Number of bytes to perform the endianness swap on
 */
export template <typename S>
  requires PlainMemoryRegisterStorage<S, S>
void SwapRegisterEndianness(std::span<std::byte> data, std::size_t offset,
                            std::size_t size) {
  using D = std::remove_cvref_t<S>;

  if (!IsAlignedRegisterAccess<D>(offset, size)) {
    return;
  }

  if constexpr (hstd::IsArray<D>) {
    using ET = typename D::value_type;
    for (std::size_t i = 0; i < data.size() / sizeof(ET); i++) {
      std::ranges::reverse(data.subspan(i * sizeof(ET), sizeof(ET)));
    }
  } else {
    std::ranges::reverse(data);
  }
}

/**
 * Performs an endianness swap on a byte array as if it were the raw contents of
 * a register, taking into account the register data type. Implementation for
 * custom register storage
 *
 * @tparam S Register storage type
 * @param data Byte view over the register data
 * @param offset Offset into the register where to perform the endianness swap
 * @param size Number of bytes to perform the endianness swap on
 */
export template <CustomReadonlyRegisterStorage S>
void SwapRegisterEndianness(std::span<std::byte> data, std::size_t offset,
                            std::size_t size) {
  S::SwapEndianness(data, offset, size);
}

namespace concepts {

/** Concept describing an input register */
export template <typename T>
concept InputRegister = IsInputRegister<T>;

/** Concept describing a holding register */
export template <typename T>
concept HoldingRegister = IsHoldingRegister<T>;

/** Concept describing a register (either input or holding register) */
export template <typename T>
concept Register = InputRegister<T> || HoldingRegister<T>;

}   // namespace concepts

}   // namespace modbus::server