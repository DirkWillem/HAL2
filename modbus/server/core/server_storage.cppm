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

export import :bit;
export import :reg;

namespace modbus::server {

template <concepts::Bits C>
consteval uint16_t StartAddress() {
  if constexpr (concepts::SingleBit<C>) {
    return C::Address;
  } else if constexpr (concepts::BitSet<C>) {
    return C::StartAddress;
  } else {
    std::unreachable();
  }
}

template <concepts::Bits C>
consteval uint16_t EndAddress() {
  if constexpr (concepts::SingleBit<C>) {
    return C::Address + 1;
  } else if constexpr (concepts::BitSet<C>) {
    return C::EndAddress;
  } else {
    std::unreachable();
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

template <typename B>
  requires concepts::DiscreteInput<B> || concepts::Coil<B>
struct BitImpl {
  using Bits = typename B::Bits;

  constexpr std::expected<uint32_t, ExceptionCode>
  Read(uint32_t mask) const noexcept {
    if constexpr (concepts::SingleBit<B>) {
      return ReadBit(storage).transform(
          [](const auto value) { return static_cast<uint32_t>(value) & 0b1U; });
    } else if constexpr (concepts::BitSet<B>) {
      return ReadBitSet(storage, static_cast<Bits>(mask))
          .transform([mask](const auto value) {
            return static_cast<uint32_t>(value) & mask;
          });
    } else {
      std::unreachable();
    }
  }

  constexpr std::expected<uint32_t, ExceptionCode>
  Write(uint32_t mask, uint32_t new_value) noexcept
    requires concepts::Coil<B>
  {
    if constexpr (concepts::SingleBit<B>) {
      return WriteBit(storage, static_cast<Bits>(new_value & 0b1U));
    } else if constexpr (concepts::BitSet<B>) {
      return WriteBitSet(storage, static_cast<Bits>(mask),
                         static_cast<Bits>(new_value));
    } else {
      std::unreachable();
    }
  }

  typename B::Storage storage{};
};

template <HoldingReg HR>
struct HoldingRegImpl {
  std::expected<std::span<const std::byte>, ExceptionCode>
  Read(std::size_t offset, std::size_t size) const noexcept {
    return ReadRegister(storage, offset, size);
  }

  std::expected<bool, ExceptionCode> Write(std::span<const std::byte> data,
                                           std::size_t offset, std::size_t size,
                                           std::endian endian) noexcept {
    if (endian != std::endian::native) {
      std::array<std::byte, sizeof(typename HR::Data)> swapped_buf{};
      std::memcpy(swapped_buf.data(), data.data(), size);
      SwapRegisterEndianness<typename HR::Storage>(swapped_buf, offset, size);

      return WriteRegister(storage, swapped_buf, offset, size);
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

template <typename T>
using BitReadFn =
    std::expected<uint32_t, ExceptionCode> (T::*)(uint32_t mask) const;
template <typename T>
using BitWriteFn = std::expected<uint32_t, ExceptionCode> (T::*)(
    uint32_t mask, uint32_t value);

template <typename T>
using RegReadFn = std::expected<std::span<const std::byte>, ExceptionCode> (
    T::*)(std::size_t offset, std::size_t size) const;
template <typename T>
using RegWriteFn = std::expected<bool, ExceptionCode> (T::*)(
    std::span<const std::byte> data, std::size_t offset, std::size_t value,
    std::endian endian);
using SwapEndiannessFn = void (*)(std::span<std::byte> data, std::size_t offset,
                                  std::size_t size);

namespace concepts {
template <typename Impl>
concept AddressRegion = requires(const Impl& impl) {
  { impl.start_addr } -> std::convertible_to<uint16_t>;
  { impl.end_addr } -> std::convertible_to<uint16_t>;
};

template <typename Impl, typename Storage>
concept ReadonlyBit = AddressRegion<Impl> && requires(const Impl& impl) {
  { impl.read } -> std::convertible_to<BitReadFn<Storage>>;
};

template <typename Impl, typename Storage>
concept MutableBit = ReadonlyBit<Impl, Storage> && requires(const Impl& impl) {
  { impl.write } -> std::convertible_to<BitWriteFn<Storage>>;
};

template <typename Impl, typename Storage>
concept ReadonlyBitArray = hstd::concepts::Array<Impl>
                           && ReadonlyBit<typename Impl::value_type, Storage>;
}   // namespace concepts

export template <typename DIs, typename Cs, typename HRs>
class ServerStorage {};

export template <concepts::DiscreteInput... UDI, concepts::Coil... UC,
                 HoldingReg... UHR>
class ServerStorage<hstd::Types<UDI...>, hstd::Types<UC...>,
                    hstd::Types<UHR...>>
    : private BitImpl<UDI>...
    , private BitImpl<UC>...
    , private HoldingRegImpl<UHR>... {
  static constexpr auto NCoils = sizeof...(UC);

  struct CoilTableEntry {
    BitReadFn<ServerStorage>  read;
    BitWriteFn<ServerStorage> write;
    uint16_t                  start_addr;
    uint16_t                  end_addr;
  };

  struct DiscreteInputTableEntry {
    BitReadFn<ServerStorage> read;
    uint16_t                 start_addr;
    uint16_t                 end_addr;
  };

  struct HoldingRegTableEntry {
    RegReadFn<ServerStorage>  read;
    RegWriteFn<ServerStorage> write;
    SwapEndiannessFn          swap_endianness;
    uint16_t                  start_addr;
    uint16_t                  end_addr;
  };

  struct InputRegTableEntry {
    RegReadFn<ServerStorage> read;
    SwapEndiannessFn         swap_endianness;
    uint16_t                 start_addr;
    uint16_t                 end_addr;
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

  using DiscreteInputs   = Reorder<UDI...>;
  using Coils            = Reorder<UC...>;
  using HoldingRegisters = Reorder<UHR...>;

  template <typename ET, hstd::concepts::Types T>
  static consteval auto BuildEntryTable(auto make_entry) {
    return ([&make_entry]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array<ET, T::Count> {
        make_entry.template operator()<typename T::template NthType<Idxs>>()...
      };
    })(std::make_index_sequence<T::Count>());
  }

  static constexpr auto DiscreteInputsTable =
      BuildEntryTable<DiscreteInputTableEntry, DiscreteInputs>(
          []<concepts::DiscreteInput DI>() {
            return DiscreteInputTableEntry{.read       = &BitImpl<DI>::Read,
                                           .start_addr = StartAddress<DI>(),
                                           .end_addr   = EndAddress<DI>()};
          });

  static constexpr auto CoilsTable =
      BuildEntryTable<CoilTableEntry, Coils>([]<concepts::Coil C>() {
        return CoilTableEntry{.read       = &BitImpl<C>::Read,
                              .write      = &BitImpl<C>::Write,
                              .start_addr = StartAddress<C>(),
                              .end_addr   = EndAddress<C>()};
      });

  static constexpr auto HoldingRegistersTable =
      BuildEntryTable<HoldingRegTableEntry, HoldingRegisters>(
          []<HoldingReg HR>() {
            return HoldingRegTableEntry{.read  = &HoldingRegImpl<HR>::Read,
                                        .write = &HoldingRegImpl<HR>::Write,
                                        .swap_endianness =
                                            &HoldingRegImpl<HR>::SwapEndianness,
                                        .start_addr = StartAddress<HR>(),
                                        .end_addr   = EndAddress<HR>()};
          });

  static_assert(ValidateNoAddressOverlap<Coils>(),
                "Coil addresses may not overlap");
  static_assert(ValidateNoAddressOverlap<HoldingRegisters>(),
                "Holding registers may not overlap");

  [[nodiscard]] static constexpr std::size_t RegToByteOffset(uint16_t offset) {
    return static_cast<std::size_t>(offset) * sizeof(uint16_t);
  }

 public:
  template <concepts::DiscreteInput DI>
  auto& GetStorage() & noexcept {
    return BitImpl<DI>::storage;
  }

  template <concepts::Coil C>
  auto& GetStorage() & noexcept {
    return BitImpl<C>::storage;
  }

  /**
   * Reads a discrete input value
   * @param address Address of the coil to read
   * @return Read coil value, or an exception code upon failure
   */
  constexpr std::expected<bool, ExceptionCode>
  ReadDiscreteInput(uint32_t address) const noexcept {
    return ReadBit<DiscreteInputsTable>(address);
  }

  /**
   * Reads multiple discrete inputs at once
   * @tparam T Bit type to return the coil values in
   * @param start_addr Address of the first coil to read
   * @param count Number of coils to read
   * @param into Byte buffer to read the coil data into
   * @return true, or exception code upon failure
   */
  constexpr std::expected<std::span<const std::byte>, ExceptionCode>
  ReadDiscreteInputs(uint16_t start_addr, uint16_t count,
                     std::span<std::byte> into) const noexcept {
    return ReadBits<DiscreteInputsTable>(start_addr, count, into);
  }

  /**
   * Reads a single coil value
   * @param address Address of the coil to read
   * @return Read coil value, or an exception code upon failure
   */
  constexpr std::expected<bool, ExceptionCode>
  ReadCoil(uint32_t address) const noexcept {
    return ReadBit<CoilsTable>(address);
  }

  /**
   * Reads multiple coils at once
   * @tparam T Bit type to return the coil values in
   * @param start_addr Address of the first coil to read
   * @param count Number of coils to read
   * @param into Byte buffer to read the coil data into
   * @return true, or exception code upon failure
   */
  constexpr std::expected<std::span<const std::byte>, ExceptionCode>
  ReadCoils(uint16_t start_addr, uint16_t count,
            std::span<std::byte> into) const noexcept {
    return ReadBits<CoilsTable>(start_addr, count, into);
  }

  /**
   * Writes a single coil
   * @param address Address of the coil to write
   * @param value Value to write
   * @return Written coil value, or an exception code upon failure
   */
  constexpr std::expected<bool, ExceptionCode> WriteCoil(uint16_t address,
                                                         bool value) noexcept {
    return WriteBit<CoilsTable>(address, value);
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
    return WriteBits<CoilsTable>(start_addr, n_coils, data);
  }

  template <std::endian E = std::endian::native>
  std::expected<std::span<const std::byte>, ExceptionCode>
  ReadHoldingRegisters(std::span<std::byte> into, uint16_t start_addr,
                       uint16_t num_regs) const noexcept {
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

  template <typename T, std::endian E = std::endian::native>
    requires(sizeof(T) % sizeof(uint16_t) == 0)
  std::expected<T, ExceptionCode>
  ReadHoldingRegister(uint16_t addr) const noexcept {
    std::array<std::byte, sizeof(T)> dst_buf{};
    const auto                       read_result =
        ReadHoldingRegisters(dst_buf, addr, sizeof(T) / sizeof(uint16_t));

    if (read_result.has_value()) {
      return std::bit_cast<T>(dst_buf);
    } else {
      return std::unexpected(read_result.error());
    }
  }

  std::expected<bool, ExceptionCode>
  WriteHoldingRegisters(std::span<const std::byte> data, uint16_t start_addr,
                        uint16_t    num_regs,
                        std::endian endian = std::endian::native) {
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
            data.subspan(src_offset, count), entry_offset, count, endian);

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

 private:
  struct AddrRange {
    uint16_t start;
    uint16_t end;
  };

  template <concepts::AddressRegion AR>
  static constexpr auto CompareAddrRangeBefore =
      [](const AR& entry, const AddrRange& search) {
        return entry.end_addr <= search.start;
      };

  template <concepts::AddressRegion AR>
  static constexpr auto CompareAddrRangeAfter =
      [](const AddrRange& search, const AR& entry) {
        return entry.start_addr >= search.end;
      };

  template <concepts::ReadonlyBitArray<ServerStorage> auto BitTable>
  constexpr std::expected<bool, ExceptionCode>
  ReadBit(uint32_t address) const noexcept {
    for (const auto& entry : BitTable) {
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

  template <concepts::ReadonlyBitArray<ServerStorage> auto BitTable>
  constexpr std::expected<std::span<const std::byte>, ExceptionCode>
  ReadBits(uint16_t start_addr, uint16_t count,
           std::span<std::byte> into) const noexcept {
    using TE = typename std::decay_t<decltype(BitTable)>::value_type;

    const uint16_t end_addr   = start_addr + count;
    const auto     read_range = AddrRange{start_addr, end_addr};

    const auto entry_start =
        std::lower_bound(BitTable.begin(), BitTable.end(), read_range,
                         CompareAddrRangeBefore<TE>);
    const auto entry_end =
        std::upper_bound(BitTable.begin(), BitTable.end(), read_range,
                         CompareAddrRangeAfter<TE>);

    if (entry_start == std::end(BitTable)) {
      return std::unexpected(ExceptionCode::IllegalDataAddress);
    }

    for (auto it = entry_start; it != entry_end; ++it) {
      const auto read_start_addr = std::max(start_addr, it->start_addr);
      const auto read_end_addr   = std::min(end_addr, it->end_addr);

      const auto entry_offset = read_start_addr - it->start_addr;
      const auto dst_offset   = read_start_addr - start_addr;
      const auto count        = read_end_addr - read_start_addr;
      const auto entry_mask   = hstd::Ones<uint32_t>(count) << entry_offset;

      const auto entry_result = (this->*it->read)(entry_mask);
      if (!entry_result.has_value()) {
        return std::unexpected(entry_result.error());
      }

      const auto entry_val = entry_result.value();

      const auto byte_offset    = dst_offset % 8;
      const auto byte_idx_start = dst_offset / 8;
      const auto byte_idx_end =
          std::max(byte_idx_start + 1, ((dst_offset + count - 1) / 8) + 1);

      for (auto i = byte_idx_start; i < byte_idx_end; ++i) {
        if (i == byte_idx_start) {
          into[i] |= static_cast<std::byte>(entry_val << byte_offset);
        } else {
          into[i] |= static_cast<std::byte>(
              entry_val >> (((i - byte_idx_start) * 8) - byte_offset));
        }
      }
    }

    return into;
  }

  template <concepts::ReadonlyBitArray<ServerStorage> auto BitTable>
  constexpr std::expected<bool, ExceptionCode> WriteBit(uint16_t address,
                                                        bool value) noexcept {
    for (const auto& entry : BitTable) {
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

  template <concepts::ReadonlyBitArray<ServerStorage> auto BitTable>
  constexpr std::expected<bool, ExceptionCode>
  WriteBits(uint16_t start_addr, uint16_t n_coils,
            std::span<const std::byte> data) noexcept {
    // For now, only byte-aligned multiple writes are supported
    if (start_addr % 8 != 0) {
      return std::unexpected(ExceptionCode::ServerDeviceFailure);
    }

    const auto end_addr = start_addr + n_coils;

    bool any_written = false;

    for (const auto& e : BitTable) {
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
};

}   // namespace modbus::server