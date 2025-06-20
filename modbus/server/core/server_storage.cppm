module;

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <expected>
#include <functional>
#include <span>
#include <tuple>
#include <utility>
#include <variant>

export module modbus.server:server_storage;

import hstd;

export import :coil;
export import :holding_register;

namespace modbus::server {

template <CoilOrCoilSet C>
consteval uint16_t StartAddress() {
  if constexpr (IsCoil<C>) {
    return C::Address;
  } else {
    return C::StartAddress;
  }
}

template <CoilOrCoilSet C>
consteval uint16_t EndAddress() {
  if constexpr (IsCoil<C>) {
    return C::Address + 1;
  } else {
    return C::EndAddress;
  }
}

template <HoldingReg H>
consteval uint16_t StartAddress() noexcept {
  return H::StartAddress;
}

template <HoldingReg H>
consteval uint16_t EndAddress() noexcept {
  return H::EndAddress;
}

template <typename Types, auto NewIdxs>
struct ReorderHelper;

template <typename... Ts, std::array<std::size_t, sizeof...(Ts)> NewIdxs>
struct ReorderHelper<hstd::Types<Ts...>, NewIdxs> {
  template <typename Idxs>
  struct Inner;

  template <std::size_t... Idxs>
  struct Inner<std::index_sequence<Idxs...>> {
    using Result = hstd::Types<
        typename hstd::Types<Ts...>::template NthType<NewIdxs[Idxs]>...>;
  };

  using Result = typename Inner<
      decltype(std::make_index_sequence<sizeof...(Ts)>())>::Result;
};

template <CoilOrCoilSet C>
struct CoilImpl {
  using Bits = typename C::Bits;

  constexpr std::expected<uint32_t, ExceptionCode>
  Read(uint32_t mask) const noexcept {
    if constexpr (IsCoil<C>) {
      return ReadCoil(storage).transform(
          [](const auto value) { return static_cast<uint32_t>(value) & 0b1U; });
    } else if constexpr (IsCoilSet<C>) {
      return ReadCoilSet(storage, static_cast<Bits>(mask))
          .transform([mask](const auto value) {
            return static_cast<uint32_t>(value) & mask;
          });
    } else {
      std::unreachable();
    }
  }

  constexpr std::expected<uint32_t, ExceptionCode>
  Write(uint32_t mask, uint32_t new_value) noexcept {
    if constexpr (IsCoil<C>) {
      return WriteCoil(storage, static_cast<Bits>(new_value & 0b1U));
    } else if constexpr (IsCoilSet<C>) {
      return WriteCoilSet(storage, static_cast<Bits>(mask),
                          static_cast<Bits>(new_value));
    } else {
      std::unreachable();
    }
  }

  typename C::Storage storage{};
};

template <HoldingReg HR>
struct HoldingRegImpl {
  std::expected<std::span<const std::byte>, ExceptionCode>
  Read(std::size_t offset, std::size_t size) const noexcept {
    return ReadRegister(storage, offset, size);
  }

  template <std::endian E = std::endian::native>
  std::expected<bool, ExceptionCode> Write(std::span<const std::byte> data,
                                           std::size_t                offset,
                                           std::size_t size) noexcept {
    if constexpr (E != std::endian::native) {
      std::array<std::byte, sizeof(typename HR::Data)> swapped_buf{};
      std::memcpy(swapped_buf.data(), data.data(), size);
      SwapEndianness<typename HR::Storage>(swapped_buf, offset, size);

      return WriteRegister(storage, data, offset, size);
    } else {
      return WriteRegister(storage, data, offset, size);
    }
  }

  static void SwapEndianness(std::span<std::byte> data, std::size_t offset,
                             std::size_t size) noexcept {
    SwapRegisterEndianness<typename HR::Storage>(data, offset, size);
  }

  typename HR::Storage storage;
};

export template <typename Cs, typename HRs>
class ServerStorage {};

export template <CoilOrCoilSet... UC, HoldingReg... UHR>
class ServerStorage<hstd::Types<UC...>, hstd::Types<UHR...>>
    : private CoilImpl<UC>...
    , private HoldingRegImpl<UHR>... {
  using CoilReadFn = std::expected<uint32_t, ExceptionCode> (ServerStorage::*)(
      uint32_t mask) const;
  using CoilWriteFn = std::expected<uint32_t, ExceptionCode> (ServerStorage::*)(
      uint32_t mask, uint32_t value);

  using RegReadFn = std::expected<std::span<const std::byte>, ExceptionCode> (
      ServerStorage::*)(std::size_t offset, std::size_t size) const;
  using RegWriteFn = std::expected<bool, ExceptionCode> (ServerStorage::*)(
      std::span<const std::byte> data, std::size_t offset, std::size_t value);
  using SwapEndiannessFn = void (*)(std::span<std::byte> data,
                                    std::size_t offset, std::size_t size);

  static constexpr auto NCoils = sizeof...(UC);

  struct CoilTableEntry {
    CoilReadFn  read;
    CoilWriteFn write;
    uint16_t    start_addr;
    uint16_t    end_addr;
  };

  struct HoldingRegTableEntry {
    RegReadFn        read;
    RegWriteFn       write;
    SwapEndiannessFn swap_endianness;
    uint16_t         start_addr;
    uint16_t         end_addr;
  };

  /**
   * Orders the server entries (coils, holding registers, ...) by address
   * @tparam Ts Entries to order
   * @return Array containing the indices of the ordered entries in the original
   * array
   */
  template <typename... Ts>
  static consteval std::array<std::size_t, sizeof...(Ts)> GetSortedOrder() {
    constexpr auto N = sizeof...(Ts);

    // Create an array containing tuples with index in the original list and
    // the start address of the entry
    auto start_addrs = ([]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array<std::tuple<std::size_t, uint16_t>, N>{
          std::make_tuple(Idxs, StartAddress<Ts>())...};
    })(std::make_index_sequence<N>());

    // Sort entries by start address
    std::ranges::sort(start_addrs, [](auto a, auto b) {
      return std::less{}(std::get<1>(a), std::get<1>(b));
    });

    // Retrieve the list indices from the sorted array
    std::array<std::size_t, N> result;
    std::ranges::transform(start_addrs, result.begin(),
                           [](const auto el) { return std::get<0>(el); });

    return result;
  }

  template <typename T>
  static consteval bool ValidateNoAddressOverlap() {
    return ([]<typename... Ts>(hstd::Types<Ts...>) {
      constexpr auto                                N = sizeof...(Ts);
      std::array<std::tuple<uint16_t, uint16_t>, N> addrs{
          std::make_tuple(StartAddress<Ts>(), EndAddress<Ts>())...};

      for (std::size_t i = 0; i < N - 1; i++) {
        const auto cur_end_addr    = std::get<1>(addrs[i]);
        const auto next_start_addr = std::get<0>(addrs[i + 1]);

        if (cur_end_addr > next_start_addr) {
          return false;
        }
      }

      return true;
    })(T{});
  }

  template <typename... Ts>
  using Reorder = typename ReorderHelper<hstd::Types<Ts...>,
                                         GetSortedOrder<Ts...>()>::Result;

  using Coils            = Reorder<UC...>;
  using HoldingRegisters = Reorder<UHR...>;

  template <hstd::concepts::Types T>
  static consteval auto BuildEntryTable(auto make_entry) {
    return ([&make_entry]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array {
        make_entry.template operator()<typename T::template NthType<Idxs>>()...
      };
    })(std::make_index_sequence<T::Count>());
  }

  static constexpr auto CoilsTable =
      BuildEntryTable<Coils>([]<CoilOrCoilSet C>() {
        return CoilTableEntry{.read       = &CoilImpl<C>::Read,
                              .write      = &CoilImpl<C>::Write,
                              .start_addr = StartAddress<C>(),
                              .end_addr   = EndAddress<C>()};
      });

  static constexpr auto HoldingRegistersTable =
      BuildEntryTable<HoldingRegisters>([]<HoldingReg HR>() {
        return HoldingRegTableEntry{
            .read            = &HoldingRegImpl<HR>::Read,
            .write           = &HoldingRegImpl<HR>::Write,
            .swap_endianness = &HoldingRegImpl<HR>::SwapEndianness,
            .start_addr      = StartAddress<HR>(),
            .end_addr        = EndAddress<HR>(),
        };
      });

  static_assert(ValidateNoAddressOverlap<Coils>(),
                "Coil addresses may not overlap");
  static_assert(ValidateNoAddressOverlap<HoldingRegisters>(),
                "Holding registers may not overlap");

  [[nodiscard]] static constexpr std::size_t RegToByteOffset(uint16_t offset) {
    return static_cast<std::size_t>(offset) * sizeof(uint16_t);
  }

 public:
  /**
   * Writes a single coil
   * @param address Address of the coil to write
   * @param value Value to write
   * @return Written coil value, or an exception code upon failure
   */
  constexpr std::expected<bool, ExceptionCode> WriteCoil(uint16_t address,
                                                         bool value) noexcept {
    for (const auto& entry : CoilsTable) {
      if (address >= entry.start_addr && address < entry.end_addr) {
        const auto     shift     = address - entry.start_addr;
        const uint32_t bit_mask  = 0b1U << shift;
        const uint32_t bit_value = value ? bit_mask : 0;

        return (this->*entry.write)(bit_mask, bit_value)
            .transform([bit_mask](const auto v) {
              return (v & bit_mask) == bit_mask;
            });
      }

      if (address < entry.start_addr) {
        return std::unexpected(ExceptionCode::IllegalDataAddress);
      }
    }

    return std::unexpected(ExceptionCode::IllegalDataAddress);
  }

  /**
   * Writes to multiple coils
   * @param start_addr Start address to write coils. Should be byte-aligned
   * @param n_coils Number of coils to write
   * @param data Data to write
   * @return true when successful, or exception code upon failure
   */
  constexpr std::expected<bool, ExceptionCode>
  WriteCoils(uint16_t start_addr, uint16_t n_coils,
             std::span<const std::byte> data) noexcept {
    // For now, only byte-aligned multiple writes are supported
    if (start_addr % 8 != 0) {
      return std::unexpected(ExceptionCode::ServerDeviceFailure);
    }

    const auto end_addr = start_addr + n_coils;

    bool any_written = false;

    for (const auto& e : CoilsTable) {
      if (start_addr < e.end_addr && end_addr >= e.start_addr) {
        const auto start_byte = e.start_addr / 8;
        const auto end_byte   = std::max(start_byte + 1, e.end_addr / 8);

        for (auto i = 0; i < (end_byte - start_byte); i++) {
          const auto byte_start_addr = start_byte * 8 + i * 8;
          const auto byte_end_addr   = start_addr + 8;

          auto byte_mask = hstd::Ones<std::byte>(8);

          if (byte_start_addr < e.start_addr) {
            const auto remove_start = e.start_addr - byte_start_addr;
            byte_mask &= ~hstd::Ones<std::byte>(remove_start);
          }

          if (byte_end_addr > e.end_addr) {
            const auto remove_end = byte_end_addr - e.end_addr;
            byte_mask &=
                ~(hstd::Ones<std::byte>(remove_end) << (8 - remove_end));
          }

          const auto word_mask  = static_cast<uint32_t>(byte_mask) << (i * 8);
          const auto word_value = static_cast<uint8_t>(data[i]) << (i * 8);

          const auto result = (this->*e.write)(word_mask, word_value);

          if (!result.has_value()) {
            return std::unexpected(result.error());
          }

          any_written = true;
        }
      }
    }

    if (!any_written) {
      return std::unexpected(ExceptionCode::IllegalDataAddress);
    }

    return true;
  }

  /**
   * Reads a single coil value
   * @param address Address of the coil to read
   * @return Read coil value, or an exception code upon failure
   */
  constexpr std::expected<bool, ExceptionCode>
  ReadCoil(uint32_t address) const noexcept {
    for (const auto& entry : CoilsTable) {
      if (address >= entry.start_addr && address < entry.end_addr) {
        const auto     shift    = address - entry.start_addr;
        const uint32_t bit_mask = 0b1U << shift;

        return (this->*entry.read)(bit_mask).transform(
            [bit_mask](const auto v) { return (v & bit_mask) == bit_mask; });
      }

      if (address < entry.start_addr) {
        return std::unexpected(ExceptionCode::IllegalDataAddress);
      }
    }

    return std::unexpected(ExceptionCode::IllegalDataAddress);
  }

  /**
   * Reads multiple (up to 32) coils at once
   * @tparam T Bit type to return the coil values in
   * @param start_address Address of the first coil to read
   * @param count Number of coils to read
   * @param ignore_none_found Whether to still succeed if no coils were found
   * @return Read coil values, or exception code upon failure
   */
  template <std::unsigned_integral T>
    requires(sizeof(T) <= sizeof(uint32_t))
  constexpr std::expected<T, ExceptionCode>
  ReadCoils(uint32_t start_address, uint32_t count,
            bool ignore_none_found = false) const noexcept {
    const auto end_address = start_address + count;

    bool any_read = false;

    T result{0};

    for (const auto& entry : CoilsTable) {
      if (start_address < entry.end_addr && end_address >= entry.start_addr) {
        any_read = true;
        const auto entry_size =
            static_cast<uint32_t>(entry.end_addr - entry.start_addr);

        if (start_address <= entry.start_addr) {
          const auto shift     = entry.start_addr - start_address;
          const auto n_to_read = std::min(entry_size, count - shift);

          const auto entry_result =
              (this->*entry.read)(hstd::Ones<uint32_t>(n_to_read));
          if (entry_result) {
            result |= static_cast<T>(*entry_result << shift);
          } else {
            return entry_result;
          }
        } else {
          const auto shift     = start_address - entry.start_addr;
          const auto n_to_read = std::min(entry_size - shift, count);

          const auto entry_result =
              (this->*entry.read)(hstd::Ones<uint32_t>(n_to_read) << shift);
          if (entry_result) {
            result |= static_cast<T>(*entry_result >> shift);
          } else {
            return entry_result;
          }
        }
      } else if (entry.start_addr >= end_address) {
        break;
      }
    }

    if (!any_read && !ignore_none_found) {
      return std::unexpected(ExceptionCode::IllegalDataAddress);
    } else {
      return result;
    }
  }

  template <std::endian E = std::endian::native>
  std::expected<std::span<const std::byte>, ExceptionCode>
  ReadHoldingRegisters(std::span<std::byte> into, uint16_t start_addr,
                       uint16_t num_regs) {
    const uint16_t end_addr = start_addr + num_regs;
    const auto     n_bytes  = num_regs * sizeof(uint16_t);

    std::fill(into.begin(), into.end(), std::byte{0});

    bool any_read = false;

    for (const auto& e : HoldingRegistersTable) {
      if (start_addr < e.end_addr && end_addr > e.start_addr) {
        const auto read_start_addr = std::max(start_addr, e.start_addr);
        const auto read_end_addr   = std::min(end_addr, e.end_addr);

        const auto entry_offset =
            RegToByteOffset(read_start_addr - e.start_addr);
        const auto dst_offset = RegToByteOffset(read_start_addr - start_addr);
        const auto count = RegToByteOffset(read_end_addr - read_start_addr);

        const auto read_result = (this->*e.read)(entry_offset, count);
        if (read_result.has_value()) {
          auto dst = into.subspan(dst_offset, count);
          std::memcpy(dst.data(), (*read_result).data(), count);

          if constexpr (E != std::endian::native) {
            (*e.swap_endianness)(dst, entry_offset, count);
          }
        } else {
          return std::unexpected(read_result.error());
        }

        any_read = true;
      } else if (e.start_addr >= end_addr) {
        break;
      }
    }

    if (!any_read) {
      return std::unexpected(ExceptionCode::IllegalDataAddress);
    }

    return into.subspan(0, n_bytes);
  }

  std::expected<bool, ExceptionCode>
  WriteHoldingRegisters(std::span<const std::byte> data, uint16_t start_addr,
                        uint16_t num_regs) {
    const uint16_t end_addr = start_addr + num_regs;

    bool any_written = false;

    for (const auto& e : HoldingRegistersTable) {
      if (start_addr < e.end_addr && end_addr > e.start_addr) {
        const auto write_start_addr = std::max(start_addr, e.start_addr);
        const auto write_end_addr   = std::min(end_addr, e.end_addr);

        const auto entry_offset =
            RegToByteOffset(write_start_addr - e.start_addr);
        const auto src_offset = RegToByteOffset(write_start_addr - start_addr);
        const auto count = RegToByteOffset(write_end_addr - write_start_addr);

        const auto write_result = (this->*e.write)(
            data.subspan(src_offset, count), entry_offset, count);

        if (!write_result.has_value()) {
          return std::unexpected(write_result.error());
        }

        any_written = true;
      } else if (e.start_addr >= end_addr) {
        break;
      }
    }

    if (!any_written) {
      return std::unexpected(ExceptionCode::IllegalDataAddress);
    }

    return true;
  }

  template <typename T>
    requires(sizeof(T) % sizeof(uint16_t) == 0)
  std::expected<bool, ExceptionCode>
  WriteHoldingRegister(const T& value, uint16_t addr) noexcept {
    return WriteHoldingRegisters(hstd::ByteViewOver(value), addr,
                                 sizeof(T) / sizeof(uint16_t));
  }
};

}   // namespace modbus::server