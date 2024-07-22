#pragma once

#include <span>

#include "callback.h"

namespace hal {

enum class I2cAddressLength { Bits7, Bits10 };

enum class I2cSpeedMode { Standard, Fast, FastPlus };

template <typename Impl>
concept AsyncI2c = requires(Impl& impl) {
  impl.I2cReceiveCallback(std::declval<std::span<std::byte>>());
  impl.I2cErrorCallback();
};

class I2cCallbacks {
 public:
  constexpr void I2cReceiveCallback(std::span<std::byte> data) const noexcept {
    if (rx_callback != nullptr) {
      (*rx_callback)(data);
    }
  }

  constexpr void RegisterI2cReceiveCallback(
      hal::Callback<std::span<std::byte>>& new_callback) noexcept {
    rx_callback = &new_callback;
  }

  constexpr void I2cErrorCallback() const noexcept {
    if (err_callback != nullptr) {
      (*err_callback)();
    }
  }

  constexpr void
  RegisterI2cErrorCallback(hal::Callback<>& new_callback) noexcept {
    err_callback = &new_callback;
  }

 private:
  hal::Callback<std::span<std::byte>>* rx_callback{nullptr};
  hal::Callback<>*                     err_callback{nullptr};
};

}   // namespace hal