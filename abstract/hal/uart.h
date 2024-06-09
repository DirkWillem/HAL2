#pragma once

#include <string_view>

#include <constexpr_tools/logic.h>

#include "callback.h"

namespace hal {

enum class UartOperatingMode { Poll, Interrupt, Dma };

enum class UartFlowControl { None };

enum class UartParity { Even, Odd, None };

enum class UartStopBits { Half, One, OneAndHalf, Two };

template <typename Impl>
concept UartBase = requires {
  { Impl::OperatingMode } -> std::convertible_to<UartOperatingMode>;
  { Impl::FlowControl } -> std::convertible_to<UartFlowControl>;
};

template <typename Impl>
concept AsyncUart = UartBase<Impl> && requires(Impl& impl) {
  impl.UartReceiveCallback(std::declval<std::span<std::byte>>());

  impl.Write(std::declval<std::string_view>());
  impl.Write(std::declval<std::span<const std::byte>>());
  impl.Receive(std::declval<std::span<std::byte>>());

  impl.RegisterUartReceiveCallback(
      std::declval<hal::Callback<std::span<std::byte>>&>());
};

template <typename Impl>
concept BlockingUart = UartBase<Impl> && requires(Impl& impl) {
  impl.WriteBlocking(std::declval<std::string_view>());
  impl.WriteBlocking(std::declval<std::span<const std::byte>>());
  impl.ReceiveBlocking(std::declval<std::span<std::byte>>());
};

template <typename Impl>
/**
 * Basic implementation for UART
 */
concept Uart =
    UartBase<Impl>
    && (ct::Implies(Impl::OperatingMode == UartOperatingMode::Interrupt
                        || Impl::OperatingMode == UartOperatingMode::Dma,
                    AsyncUart<Impl>));

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
      (*callback)(data);
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