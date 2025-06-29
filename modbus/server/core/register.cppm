module;

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <expected>
#include <ranges>
#include <span>
#include <string_view>

export module modbus.server:reg;

import hstd;

import modbus.core;

namespace modbus::server {

/**
 * Concept for a plain memory holding register storage
 * @tparam T Checked type
 * @tparam D Data type
 */
template <typename T, typename D>
concept PlainMemoryRegisterStorage =
    (std::same_as<T, D> && std::is_trivially_copyable_v<D>
     && sizeof(T) % sizeof(uint16_t) == 0);

/**
 * Concept for the mutable storage of a register
 * @tparam T Holding Register type
 * @tparam D Type of the stored data
 */
export template <typename T, typename D>
concept MutableRegisterStorage = PlainMemoryRegisterStorage<T, D>;

/**
 * Concept for the read-only storage of a register
 * @tparam T Holding Register type
 * @tparam D Type of the stored data
 */
export template <typename T, typename D>
concept ReadonlyRegisterStorage = MutableRegisterStorage<T, D>;

/**
 * Describes an input register
 * @tparam A Address
 * @tparam D Stored data type
 * @tparam S Storage implementation
 * @tparam N Name of the register
 */
export template <uint16_t A, typename D, ReadonlyRegisterStorage<D> S,
                 hstd::StaticString N>
struct InputRegister {
  using Data    = D;
  using Storage = S;

  static constexpr auto             StartAddress = A;
  static constexpr auto             EndAddress   = A + (sizeof(D) / 2);
  static constexpr std::string_view Name         = N;
};

template <typename T>
inline constexpr bool IsInputRegister = false;

template <uint16_t A, typename D, MutableRegisterStorage<D> S,
          hstd::StaticString N>
inline constexpr bool IsInputRegister<InputRegister<A, D, S, N>> = true;

/**
 * Type alias for an in-memory input register
 * @tparam A Address
 * @tparam D Stored data type
 * @tparam N Name of the register
 */
export template <uint16_t A, typename D, hstd::StaticString N>
using InMemInputRegister = InputRegister<A, D, D, N>;

/**
 * Describes a holding register
 * @tparam A Address
 * @tparam D Stored data type
 * @tparam S Storage implementation
 * @tparam N Name of the register
 */
export template <uint16_t A, typename D, MutableRegisterStorage<D> S,
                 hstd::StaticString N>
struct HoldingRegister {
  using Data    = D;
  using Storage = S;

  static constexpr auto             StartAddress = A;
  static constexpr auto             EndAddress   = A + (sizeof(D) / 2);
  static constexpr std::string_view Name         = N;
};

template <typename T>
inline constexpr bool IsHoldingRegister = false;

template <uint16_t A, typename D, MutableRegisterStorage<D> S,
          hstd::StaticString N>
inline constexpr bool IsHoldingRegister<HoldingRegister<A, D, S, N>> = true;

/**
 * Type alias for an in-memory holding register
 * @tparam A Address
 * @tparam D Stored data type
 * @tparam N Name of the register
 */
export template <uint16_t A, typename D, hstd::StaticString N>
using InMemHoldingRegister = HoldingRegister<A, D, D, N>;

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

namespace concepts {

export template <typename T>
concept InputRegister = IsInputRegister<T>;

export template <typename T>
concept HoldingRegister = IsHoldingRegister<T>;

export template <typename T>
concept Register = InputRegister<T> || HoldingRegister<T>;

}   // namespace concepts

}   // namespace modbus::server