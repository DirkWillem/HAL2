module;

#include <chrono>
#include <optional>
#include <span>
#include <string_view>

export module hal.abstract:uart;

import hstd;

import :peripheral;

namespace hal {

export enum class UartOperatingMode { Poll, Interrupt, Dma };

export enum class UartFlowControl { None };

export enum class UartParity { Even, Odd, None };

export enum class UartStopBits { Half, One, OneAndHalf, Two };

export template <typename Impl>
concept UartBase = Peripheral<Impl> && IsPeripheralInUse<Impl>();

export template <typename Impl>
concept AsyncUart = UartBase<Impl> && requires(Impl& impl) {
  impl.UartReceiveCallback(std::declval<std::span<std::byte>>());
  impl.UartTransmitCallback();

  impl.Write(std::declval<std::string_view>());
  impl.Write(std::declval<std::span<const std::byte>>());
  impl.Receive(std::declval<std::span<std::byte>>());

  impl.RegisterUartReceiveCallback(
      std::declval<hstd::Callback<std::span<std::byte>>&>());
  impl.ClearUartReceiveCallback();

  impl.RegisterUartTransmitCallback(std::declval<hstd::Callback<>&>());
  impl.ClearUartTransmitCallback();
};

export template <typename Impl>
concept RtosUart = UartBase<Impl> && requires(Impl& impl) {
  {
    impl.Write(std::declval<std::string_view>(),
               std::declval<std::chrono::milliseconds>())
  } -> std::convertible_to<bool>;
  {
    impl.Write(std::declval<std::span<const std::byte>>(),
               std::declval<std::chrono::milliseconds>())
  } -> std::convertible_to<bool>;
  {
    impl.Receive(std::declval<std::span<std::byte>>(),
                 std::declval<std::chrono::milliseconds>())
  } -> std::convertible_to<std::optional<std::span<std::byte>>>;
};

export template <typename Impl>
concept BlockingUart = UartBase<Impl> && requires(Impl& impl) {
  impl.WriteBlocking(std::declval<std::string_view>());
  impl.WriteBlocking(std::declval<std::span<const std::byte>>());
  impl.ReceiveBlocking(std::declval<std::span<std::byte>>());
};

export template <typename Impl>
/**
 * Basic implementation for UART
 */
concept Uart =
    UartBase<Impl>
    && (hstd::Implies(Impl::OperatingMode == UartOperatingMode::Interrupt
                          || Impl::OperatingMode == UartOperatingMode::Dma,
                      AsyncUart<Impl>));

/**
 * Helper class for adding registration of UART receive callbacks to an UART
 * instance
 */
export class RegisterableUartReceiveCallback {
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
      hstd::Callback<std::span<std::byte>>& new_callback) noexcept {
    callback = &new_callback;
  }

  constexpr void ClearUartReceiveCallback() { callback = nullptr; }

 private:
  hstd::Callback<std::span<std::byte>>* callback{nullptr};
};

export class RegisterableUartTransmitCallback {
 public:
  constexpr void UartTransmitCallback() {
    if (callback != nullptr) {
      (*callback)();
    }
  }

  constexpr void
  RegisterUartTransmitCallback(hstd::Callback<>& new_callback) noexcept {
    callback = &new_callback;
  }

  constexpr void ClearUartTransmitCallback() noexcept { callback = nullptr; }

 private:
  hstd::Callback<>* callback{nullptr};
};

}   // namespace hal
