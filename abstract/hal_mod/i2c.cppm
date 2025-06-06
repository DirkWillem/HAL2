module;

#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

export module hal.abstract:i2c;

import hstd;

namespace hal {

export enum class I2cAddressLength { Bits7, Bits10 };

export enum class I2cSpeedMode { Standard, Fast, FastPlus };

export template <typename T>
concept I2cMemAddr = (std::is_same_v<std::decay_t<T>, uint8_t>
                      || std::is_same_v<std::decay_t<T>, uint16_t>);

export [[nodiscard]] consteval I2cMemAddr auto I2cMemAddr8(uint8_t addr) noexcept {
  return addr;
}

export [[nodiscard]] consteval I2cMemAddr auto I2cMemAddr16(uint16_t addr) noexcept {
  return addr;
}

template <typename Impl>
concept AsyncI2cCallbacks = requires(Impl& impl) {
  impl.I2cReceiveCallback(std::declval<uint16_t>(),
                          std::declval<std::span<std::byte>>());
  impl.I2cTransmitCallback(std::declval<uint16_t>());
  impl.I2cMemReadCallback(std::declval<uint16_t>(), std::declval<uint16_t>(),
                          std::declval<std::span<std::byte>>());
  impl.I2cMemWriteCallback(std::declval<uint16_t>(), std::declval<uint16_t>());

  impl.I2cErrorCallback();
};

export template <typename Impl>
concept AsyncI2c = AsyncI2cCallbacks<Impl> && requires(Impl& impl) {
  impl.Transmit(std::declval<uint16_t>(), std::declval<std::span<std::byte>>());

  impl.ReadMemory(std::declval<uint16_t>(), std::declval<uint16_t>(),
                  std::declval<std::span<std::byte>>());
  impl.ReadMemory(std::declval<uint16_t>(), std::declval<uint16_t>(),
                  std::declval<std::span<std::byte>>(),
                  std::declval<std::optional<std::size_t>>());
  impl.WriteMemory(std::declval<uint16_t>(), std::declval<uint16_t>(),
                   std::declval<std::span<std::byte>>());
  impl.WriteMemoryValue(std::declval<uint16_t>(), std::declval<uint16_t>(),
                        std::declval<std::byte>());
};

export template <typename Impl>
concept AsyncI2cRegisterableCallbacks = requires(Impl& impl) {
  impl.RegisterI2cReceiveCallback(
      std::declval<hstd::Callback<uint16_t, std::span<std::byte>>&>());
  impl.RegisterI2cTransmitCallback(std::declval<hstd::Callback<uint16_t>&>());
};

export class I2cCallbacks {
 public:
  constexpr void I2cErrorCallback() const noexcept {
    if (err_callback != nullptr) {
      (*err_callback)();
    }
  }

  constexpr void
  RegisterI2cErrorCallback(hstd::Callback<>& new_callback) noexcept {
    err_callback = &new_callback;
  }

  constexpr void I2cReceiveCallback(uint16_t             dev_addr,
                                    std::span<std::byte> data) const noexcept {
    if (rx_callback != nullptr) {
      (*rx_callback)(dev_addr, data);
    }
  }

  constexpr void RegisterI2cReceiveCallback(
      hstd::Callback<uint16_t, std::span<std::byte>>& new_callback) noexcept {
    rx_callback = &new_callback;
  }

  constexpr void I2cTransmitCallback(uint16_t dev_addr) const noexcept {
    if (tx_callback != nullptr) {
      (*tx_callback)(dev_addr);
    }
  }

  constexpr void RegisterI2cTransmitCallback(
      hstd::Callback<uint16_t>& new_callback) noexcept {
    tx_callback = &new_callback;
  }

  constexpr void RegisterI2cMemReadCallback(
      hstd::Callback<uint16_t, uint16_t, std::span<std::byte>>&
          new_callback) {
    mem_read_callback = &new_callback;
  }

  constexpr void I2cMemReadCallback(uint16_t dev_addr, uint16_t mem_addr,
                                    std::span<std::byte> data) {
    if (mem_read_callback != nullptr) {
      (*mem_read_callback)(dev_addr, mem_addr, data);
    }
  }

  constexpr void I2cMemWriteCallback(uint16_t dev_addr,
                                     uint16_t mem_addr) const noexcept {
    if (mem_write_callback != nullptr) {
      (*mem_write_callback)(dev_addr, mem_addr);
    }
  }

  constexpr void RegisterI2cMemWriteCallback(
      hstd::Callback<uint16_t, uint16_t>& new_callback) noexcept {
    mem_write_callback = &new_callback;
  }

 private:
  hstd::Callback<>* err_callback{nullptr};

  hstd::Callback<uint16_t, std::span<std::byte>>* rx_callback{nullptr};
  hstd::Callback<uint16_t>*                       tx_callback{nullptr};
  hstd::Callback<uint16_t, uint16_t, std::span<std::byte>>* mem_read_callback{
      nullptr};
  hstd::Callback<uint16_t, uint16_t>* mem_write_callback{nullptr};
};

static_assert(AsyncI2cCallbacks<I2cCallbacks>);

}   // namespace hal
