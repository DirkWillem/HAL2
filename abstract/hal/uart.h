#pragma once

#include <string_view>

#include "callback.h"

namespace hal {

enum class UartOperatingMode { Poll, Interrupt, Dma };

enum class UartFlowControl { None };

enum class UartParity { Even, Odd, None };

enum class UartStopBits { Half, One, OneAndHalf, Two };

template <typename Impl>
/**
 * Basic implementation for UART
 */
concept Uart = requires(Impl& impl) {
  impl.Write(std::declval<std::string_view>());
  impl.Write(std::declval<std::span<const std::byte>>());
  impl.Receive(std::declval<std::span<std::byte>>());
};

template <typename Impl>
/**
 * Concept for UART implementations that implement the receive callback
 */
concept UartReceiveCallback = requires(Impl& impl) {
  impl.UartReceiveCallback(std::declval<std::span<std::byte>>());
};

template <typename Impl>
concept UartRegisterReceiveCallback =
    UartReceiveCallback<Impl> && requires(Impl& impl) {
      impl.RegisterUartReceiveCallback(
          std::declval<hal::Callback<std::span<std::byte>>&>());
    };

/**
 * Helper class for adding registration of UART receive callbacks to an UART
 * instance
 */
class RegisterableUartReceiveCallback {
 public:
  /**
   * UART receive callback implementation
   * @param data Received data
   */
  constexpr void UartReceiveCallback(std::span<std::byte> data) noexcept {
    if (callback != nullptr) {
      callback->Invoke(data);
    }
  }

  /**
   * Registers the UART receive callback
   * @param new_callback New receive callback to register
   */
  constexpr void RegisterUartReceiveCallback(
      hal::Callback<std::span<std::byte>>& new_callback) noexcept {
    callback = &new_callback;
  }

 private:
  hal::Callback<std::span<std::byte>>* callback{nullptr};
};

}   // namespace hal