#pragma once

#include <cassert>
#include <span>

#include <stm32g0xx_hal.h>

#include <halstd/compile_time_assert.h>
#include <halstd/spans.h>

#include <hal/peripheral.h>
#include <hal/uart.h>

#include <stm32g0/dma.h>
#include <stm32g0/pin.h>

#include <stm32g0/mappings/uart_pin_mapping.h>

extern "C" {

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size);
}

namespace stm32g0 {

enum class UartFlowControl {
  None,
  Rs485,
};

enum class Rs485DriveEnablePolarity : uint32_t {
  High = UART_DE_POLARITY_HIGH,
  Low  = UART_DE_POLARITY_LOW
};

/**
 * Struct containing all configuration parameters of a UART peripheral
 */
struct UartConfig {
  hal::UartParity   parity    = hal::UartParity::None;
  hal::UartStopBits stop_bits = hal::UartStopBits::One;

  UartFlowControl          flow_control = UartFlowControl::None;
  Rs485DriveEnablePolarity de_polarity  = Rs485DriveEnablePolarity::High;
  uint8_t                  rs485_assertion_time   = 0;
  uint8_t                  rs485_deassertion_time = 0;

  consteval bool Validate() const noexcept {
    // Validate RS-485-related parameters
    halstd::Assert(rs485_assertion_time <= 31,
                   "RS-485 assertion time must be between 0 and 31");
    halstd::Assert(rs485_deassertion_time <= 31,
                   "RS-485 assertion time must be between 0 and 31");

    return true;
  }
};

namespace detail {

template <UartId Id, UartConfig C>
struct UartPinoutHelper;

template <UartId Id, UartConfig C>
  requires(C.flow_control == UartFlowControl::None)
struct UartPinoutHelper<Id, C> {
  struct Pinout {
    consteval Pinout(PinId tx, PinId rx,
                     hal::PinPull pull_tx = hal::PinPull::NoPull,
                     hal::PinPull pull_rx = hal::PinPull::NoPull) noexcept
        : tx{tx}
        , rx{rx}
        , pull_tx{pull_tx}
        , pull_rx{pull_rx} {
      halstd::Assert(
          hal::FindPinAFMapping(UartTxPinMappings, Id, tx).has_value(),
          "TX pin must be valid");
      halstd::Assert(
          hal::FindPinAFMapping(UartRxPinMappings, Id, rx).has_value(),
          "RX pin must be valid");
    }

    void Initialize() noexcept {
      Pin::InitializeAlternate(
          tx, hal::FindPinAFMapping(UartTxPinMappings, Id, tx)->af, pull_tx);
      Pin::InitializeAlternate(
          rx, hal::FindPinAFMapping(UartRxPinMappings, Id, rx)->af, pull_rx);
    }

    PinId tx;
    PinId rx;

    hal::PinPull pull_tx;
    hal::PinPull pull_rx;
  };
};

template <UartId Id, UartConfig C>
  requires(C.flow_control == UartFlowControl::Rs485)
struct UartPinoutHelper<Id, C> {
  struct Pinout {
    consteval Pinout(PinId tx, PinId rx, PinId de,
                     hal::PinPull pull_tx = hal::PinPull::NoPull,
                     hal::PinPull pull_rx = hal::PinPull::NoPull,
                     hal::PinPull pull_de = hal::PinPull::NoPull) noexcept
        : tx{tx}
        , rx{rx}
        , de{de}
        , pull_tx{pull_tx}
        , pull_rx{pull_rx}
        , pull_de{pull_de} {
      halstd::Assert(
          hal::FindPinAFMapping(UartTxPinMappings, Id, tx).has_value(),
          "TX pin must be valid");
      halstd::Assert(
          hal::FindPinAFMapping(UartRxPinMappings, Id, rx).has_value(),
          "RX pin must be valid");
      halstd::Assert(
          hal::FindPinAFMapping(UartRtsPinMappings, Id, de).has_value(),
          "DE pin must be valid");
    }

    void Initialize() noexcept {
      Pin::InitializeAlternate(
          tx, hal::FindPinAFMapping(UartTxPinMappings, Id, tx)->af, pull_tx);
      Pin::InitializeAlternate(
          rx, hal::FindPinAFMapping(UartRxPinMappings, Id, rx)->af, pull_rx);
      Pin::InitializeAlternate(
          de, hal::FindPinAFMapping(UartRtsPinMappings, Id, de)->af, pull_de);
    }

    PinId tx;
    PinId rx;
    PinId de;

    hal::PinPull pull_tx;
    hal::PinPull pull_rx;
    hal::PinPull pull_de;
  };
};

void SetupUartNoFc(UartId id, UART_HandleTypeDef& huart, unsigned baud,
                   const UartConfig& cfg) noexcept;

void SetupUartRs485(UartId id, UART_HandleTypeDef& huart, unsigned baud,
                    const UartConfig& cfg) noexcept;

void InitializeUartForPollMode(UART_HandleTypeDef& huart) noexcept;
void InitializeUartForInterruptMode(UartId              id,
                                    UART_HandleTypeDef& huart) noexcept;

}   // namespace detail

/**
 * UART Transmit DMA channel
 * @tparam Id UART Id
 * @tparam Prio DMA Priority
 */
template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartTxDma = DmaChannel<Id, UartDmaRequest::Tx, Prio>;

/**
 * UART Receive DMA channel
 * @tparam Id UART Id
 * @tparam Prio DMA Priority
 */
template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartRxDma = DmaChannel<Id, UartDmaRequest::Rx, Prio>;

template <typename Impl, UartId Id,
          hal::UartOperatingMode OM = hal::UartOperatingMode::Poll,
          UartConfig             C  = {}>
/**
 * Implementation for UART
 * @tparam Id UART Id
 * @tparam FC UART Flow Control
 * @tparam OM UART Operating Mode
 */
class UartImpl : public hal::UsedPeripheral {
  friend void ::HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart,
                                           uint16_t            size);
  friend void ::HAL_UART_TxCpltCallback(UART_HandleTypeDef*);

  static_assert(C.Validate());

 public:
  static constexpr auto OperatingMode = OM;
  using TxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Tx>;
  using RxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Rx>;
  using Pinout = typename detail::UartPinoutHelper<Id, C>::Pinout;

  void HandleInterrupt() noexcept { HAL_UART_IRQHandler(&huart); }

  /**
   * Writes a string to the UART
   * @param sv View of the string to write
   */
  void Write(std::string_view sv)
    requires(OM == hal::UartOperatingMode::Poll)
  {
    HAL_UART_Transmit(&huart, halstd::ReinterpretSpan<uint8_t>(sv).data(),
                      sv.size(), 500);
  }

  void Write(std::string_view sv)
    requires(OM == hal::UartOperatingMode::Interrupt)
  {
    HAL_UART_Transmit_IT(&huart, halstd::ReinterpretSpan<uint8_t>(sv).data(),
                         sv.size());
  }

  void Write(std::string_view sv)
    requires(OM == hal::UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, halstd::ReinterpretSpan<uint8_t>(sv).data(),
                          sv.size());
  }

  void Write(std::span<const std::byte> data)
    requires(OM == hal::UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, halstd::ReinterpretSpan<uint8_t>(data).data(),
                          data.size());
  }

  void Receive(std::span<std::byte> into) noexcept
    requires(OM == hal::UartOperatingMode::Dma)
  {
    rx_buf = into;
    HAL_UARTEx_ReceiveToIdle_DMA(
        &huart, halstd::ReinterpretSpanMut<uint8_t>(rx_buf).data(),
        rx_buf.size());
  }

  /**
   * Singleton constructor
   * @return Singleton instance
   */
  [[nodiscard]] static Impl& instance() noexcept {
    static Impl inst{};
    return inst;
  }

 protected:
  /**
   * Constructor for UART without flow control in poll or interrupt mode
   * @param pinout UART pinout
   * @param baud UART baud rate
   */
  UartImpl(Pinout pinout, unsigned baud)
    requires(OM != hal::UartOperatingMode::Dma)
      : huart{} {
    // Set up pins
    pinout.Initialize();

    // Initialize UART
    if constexpr (C.flow_control == UartFlowControl::None) {
      detail::SetupUartNoFc(Id, huart, baud, C);
    } else if constexpr (C.flow_control == UartFlowControl::Rs485) {
      detail::SetupUartRs485(Id, huart, baud, C);
    }

    // Set up UART for the requested operation mode
    if constexpr (OM == hal::UartOperatingMode::Poll) {
      detail::InitializeUartForPollMode(huart);
    } else if constexpr (OM == hal::UartOperatingMode::Interrupt) {
      detail::InitializeUartForInterruptMode(Id, huart);
    } else {
      std::unreachable();
    }
  }

  void ReceiveComplete(std::size_t n_bytes) noexcept {
    if constexpr (hal::AsyncUart<Impl>) {
      static_cast<Impl*>(this)->UartReceiveCallback(rx_buf.subspan(0, n_bytes));
    }
  }

  void TransmitComplete() noexcept {
    if constexpr (hal::AsyncUart<Impl>) {
      static_cast<Impl*>(this)->UartTransmitCallback();
    }
  }

  /**
   * Constructor for UART without flow control in DMA mode
   * @param pinout UART pinout
   * @param baud UART baud rate
   */
  UartImpl(hal::Dma auto& dma, Pinout pinout, unsigned baud)
    requires(OM == hal::UartOperatingMode::Dma)
      : huart{} {
    using Dma = std::decay_t<decltype(dma)>;
    static_assert(Dma::template ChannelEnabled<TxDmaChannel>(),
                  "TX DMA channel must be enabled");
    static_assert(Dma::template ChannelEnabled<RxDmaChannel>(),
                  "RX DMA channel must be enabled");

    // Set up pins
    pinout.Initialize();

    // Initialize UART
    if constexpr (C.flow_control == UartFlowControl::None) {
      detail::SetupUartNoFc(Id, huart, baud, C);
    } else if constexpr (C.flow_control == UartFlowControl::Rs485) {
      detail::SetupUartRs485(Id, huart, baud, C);
    }

    // Set up UART for the requested operation mode
    auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
        hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&huart, hdmatx, htxdma);

    auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
        hal::DmaDirection::PeriphToMem, hal::DmaMode::Normal,
        hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&huart, hdmarx, hrxdma);

    detail::InitializeUartForInterruptMode(Id, huart);
  }

  std::span<std::byte> rx_buf{};
  UART_HandleTypeDef   huart;
};

template <UartId Id>
/**
 * Marker class for UART peripherals
 * @tparam Id UART id
 */
class Uart : public hal::UnusedPeripheral<Uart<Id>> {
  friend void ::HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart,
                                           uint16_t            size);
  friend void ::HAL_UART_TxCpltCallback(UART_HandleTypeDef*);

 public:
  constexpr void HandleInterrupt() noexcept {}

 protected:
  void ReceiveComplete(std::size_t) noexcept {}
  void TransmitComplete() noexcept {}

  UART_HandleTypeDef huart{};
};

using Usart1  = Uart<UartId::Usart1>;
using Usart2  = Uart<UartId::Usart2>;
using Usart3  = Uart<UartId::Usart3>;
using Usart4  = Uart<UartId::Usart4>;
using Usart5  = Uart<UartId::Usart5>;
using Usart6  = Uart<UartId::Usart6>;
using LpUart1 = Uart<UartId::LpUart1>;
using LpUart2 = Uart<UartId::LpUart2>;

}   // namespace stm32g0