module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <span>
#include <tuple>
#include <utility>
#include <variant>

export module modbus.server;

import hstd;

export import :coil;

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

template <typename Types, auto NewIdxs>
using Reorder = typename ReorderHelper<Types, NewIdxs>::Result;

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

export template <typename Coils>
class Server {};

export template <CoilOrCoilSet... UC>
class Server<hstd::Types<UC...>> : private CoilImpl<UC>... {
  using ReadFn =
      std::expected<uint32_t, ExceptionCode> (Server::*)(uint32_t mask) const;
  using WriteFn = std::expected<uint32_t, ExceptionCode> (Server::*)(
      uint32_t mask, uint32_t value);

  static constexpr auto NCoils = sizeof...(UC);

  struct CoilTableEntry {
    ReadFn   read;
    WriteFn  write;
    uint16_t start_addr;
    uint16_t end_addr;
  };

  template <typename... Ts>
  static consteval std::array<std::size_t, sizeof...(Ts)> GetSortedOrder() {
    constexpr auto N = sizeof...(Ts);

    auto start_addrs = ([]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array<std::tuple<std::size_t, uint16_t>, N>{
          std::make_tuple(Idxs, StartAddress<UC>())...};
    })(std::make_index_sequence<N>());

    std::ranges::sort(start_addrs, [](auto a, auto b) {
      return std::less{}(std::get<1>(a), std::get<1>(b));
    });

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

  using Coils = Reorder<hstd::Types<UC...>, GetSortedOrder<UC...>()>;

  template <CoilOrCoilSet C>
  static consteval CoilTableEntry MakeCoilTableEntry() {
    return CoilTableEntry{
        .read       = &CoilImpl<C>::Read,
        .write      = &CoilImpl<C>::Write,
        .start_addr = StartAddress<C>(),
        .end_addr   = EndAddress<C>(),
    };
  }

  static consteval auto BuildCoilsTable() {
    return ([]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
      return std::array<CoilTableEntry, NCoils>{
          MakeCoilTableEntry<typename Coils::template NthType<Idxs>>()...};
    })(std::make_index_sequence<NCoils>());
  }

  static constexpr auto CoilsTable = BuildCoilsTable();

  static_assert(ValidateNoAddressOverlap<Coils>(),
                "Coil addresses may not overlap");

  class FrameHandler {
    template <typename T, T Div>
    static constexpr T DivCeil(T lhs) {
      return lhs % Div == 0 ? lhs / Div : lhs / Div + 1;
    }

   public:
    constexpr FrameHandler(Server&                            server,
                           ResponsePdu<FrameVariant::Encode>& response)
        : server{server}
        , response{response}
        , buffer{server.buffer} {}

    /**
     * Handles a Read Coils request
     * @param req Request to handle
     */
    void operator()(const ReadCoilsRequest& req) noexcept {
      const auto n_bytes = DivCeil<uint16_t, 8>(req.num_coils);

      bool any_read = false;

      for (uint32_t i = 0; i < n_bytes; i++) {
        const auto n_bits_i = std::min(8U, req.num_coils - i * 8);
        const auto result = server.ReadCoils<uint8_t>(req.starting_addr + i * 8,
                                                      n_bits_i, any_read);

        if (result) {
          buffer[i] = static_cast<std::byte>(*result);
          any_read  = true;
        } else {
          response = MakeErrorResponse(req.FC, result.error());
          return;
        }
      }

      response = ReadCoilsResponse{.coils = buffer.subspan(0, n_bytes)};
    }

    /**
     * Handles a MODBUS Write Single Coil request
     * @param req Request to handle
     */
    void operator()(const WriteSingleCoilRequest& req) noexcept {
      using enum CoilState;

      if (req.new_state != Disabled && req.new_state != Enabled) {
        response = IllegalDataValue(WriteSingleCoilRequest::FC);
        return;
      }

      const auto result =
          server.WriteCoil(req.coil_addr, req.new_state == Enabled);
      if (result) {
        response = WriteSingleCoilResponse{.coil_addr = req.coil_addr,
                                           .new_state = req.new_state};
      } else {
        response = MakeErrorResponse(req.FC, result.error());
      }
    }

    /**
     * Handles a MODBUS write multiple coils request
     * @param req Request to handle
     */
    void operator()(const WriteMultipleCoilsRequest& req) noexcept {
      // Validate that the number of coils to write matches the amount of bytes
      if (req.num_coils < 8 * (req.values.size() - 1) + 1
          || req.num_coils > req.values.size() * 8) {
        response = IllegalDataValue(WriteMultipleCoilsRequest::FC);
        return;
      }

      // Write coils
      const auto result =
          server.WriteCoils(req.start_addr, req.num_coils, req.values);
      if (!result.has_value()) {
        response = MakeErrorResponse(req.FC, result.error());
        return;
      }

      // Create response
      response = WriteMultipleCoilsResponse{
          .start_addr = req.start_addr,
          .num_coils  = req.num_coils,
      };
    }

    void operator()(const auto&) noexcept {}

   private:
    Server&                            server;
    ResponsePdu<FrameVariant::Encode>& response;

    std::span<std::byte> buffer;
  };

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

  void HandleFrame(const RequestPdu<FrameVariant::Decode>& request,
                   ResponsePdu<FrameVariant::Encode>&      response) {
    std::visit(FrameHandler{*this, response}, request);
  }

 private:
  std::array<std::byte, 256> buffer{};
};

namespace concepts {

template <typename T>
inline constexpr bool IsServer = false;

template <typename UC>
inline constexpr bool IsServer<Server<UC>> = true;

export template <typename T>
concept Server = IsServer<T>;

}   // namespace concepts

}   // namespace modbus::server