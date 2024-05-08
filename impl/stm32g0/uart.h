#pragma once

#include <cassert>
#include <span>

#include <stm32g0xx_hal.h>

#include <hal/peripheral.h>
#include <hal/uart.h>

#include <stm32g0/dma.h>
#include <stm32g0/pin.h>

#include <stm32g0/mappings/uart_pin_mapping.h>

extern "C" {

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size);
}

namespace stm32g0 {

namespace detail {

template <UartId Id, hal::UartFlowControl FC>
struct UartPinoutHelper;

template <UartId Id>
struct UartPinoutHelper<Id, hal::UartFlowControl::None> {
  struct Pinout {
    consteval Pinout(PinId tx, PinId rx,
                     hal::PinPull pull_tx = hal::PinPull::NoPull,
                     hal::PinPull pull_rx = hal::PinPull::NoPull) noexcept
        : tx{tx}
        , rx{rx}
        , pull_tx{pull_tx}
        , pull_rx{pull_rx} {
      assert(("TX pin must be valid",
              hal::FindPinAFMapping(UartTxPinMappings, Id, tx).has_value()));
      assert(("RX pin must be valid",
              hal::FindPinAFMapping(UartRxPinMappings, Id, rx).has_value()));
    }

    PinId tx;
    PinId rx;

    hal::PinPull pull_tx;
    hal::PinPull pull_rx;
  };
};

void SetupUartNoFc(UartId id, UART_HandleTypeDef& huart, unsigned baud,
                   hal::UartParity   parity,
                   hal::UartStopBits stop_bits) noexcept;

void InitializeUartForPollMode(UART_HandleTypeDef& huart) noexcept;
void InitializeUartForInterruptMode(UartId              id,
                                    UART_HandleTypeDef& huart) noexcept;

}   // namespace detail

template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartTxDma = DmaChannel<Id, UartDmaRequest::Tx, Prio>;

template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartRxDma = DmaChannel<Id, UartDmaRequest::Rx, Prio>;

template <typename Impl, UartId Id,
          hal::UartOperatingMode OM = hal::UartOperatingMode::Poll,
          hal::UartFlowControl   FC = hal::UartFlowControl::None>
/**
 * Implementation for UART
 * @tparam Id UART Id
 * @tparam FC UART Flow Control
 * @tparam OM UART Operating Mode
 */
class UartImpl : public hal::UsedPeripheral {
  friend void ::HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart,
                                           uint16_t            size);

 public:
  static constexpr auto OperatingMode = OM;
  static constexpr auto FlowControl   = FC;
  using TxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Tx>;
  using RxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Rx>;
  using Pinout = detail::UartPinoutHelper<Id, FC>::Pinout;

  void HandleInterrupt() noexcept { HAL_UART_IRQHandler(&huart); }

  /**
   * Writes a string to the UART
   * @param sv View of the string to write
   */
  void Write(std::string_view sv)
    requires(OM == hal::UartOperatingMode::Poll)
  {
    HAL_UART_Transmit(&huart, reinterpret_cast<const uint8_t*>(sv.data()),
                      sv.size(), 500);
  }

  void Write(std::string_view sv)
    requires(OM == hal::UartOperatingMode::Interrupt)
  {
    HAL_UART_Transmit_IT(&huart, reinterpret_cast<const uint8_t*>(sv.data()),
                         sv.size());
  }

  void Write(std::string_view sv)
    requires(OM == hal::UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, reinterpret_cast<const uint8_t*>(sv.data()),
                          sv.size());
  }

  void Write(std::span<const std::byte> data)
    requires(OM == hal::UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, reinterpret_cast<const uint8_t*>(data.data()),
                          data.size());
  }

  void Receive(std::span<std::byte> into) noexcept
    requires(OM == hal::UartOperatingMode::Dma)
  {
    rx_buf = into;
    HAL_UARTEx_ReceiveToIdle_DMA(
        &huart, reinterpret_cast<uint8_t*>(rx_buf.data()), rx_buf.size());
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
   * @param parity UART parity
   * @param stop_bits UART stop bits
   */
  UartImpl(Pinout pinout, unsigned baud,
           hal::UartParity   parity    = hal::UartParity::None,
           hal::UartStopBits stop_bits = hal::UartStopBits::One)
    requires(FC == hal::UartFlowControl::None
             && OM != hal::UartOperatingMode::Dma)
      : huart{} {
    // Set up tx and rx pins
    Pin::InitializeAlternate(
        pinout.tx, hal::FindPinAFMapping(UartTxPinMappings, Id, pinout.tx)->af,
        pinout.pull_tx);
    Pin::InitializeAlternate(
        pinout.rx, hal::FindPinAFMapping(UartRxPinMappings, Id, pinout.rx)->af,
        pinout.pull_rx);

    // Initialize UART
    detail::SetupUartNoFc(Id, huart, baud, parity, stop_bits);

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

  /**
   * Constructor for UART without flow control in DMA mode
   * @param pinout UART pinout
   * @param baud UART baud rate
   * @param parity UART parity
   * @param stop_bits UART stop bits
   */
  UartImpl(hal::Dma auto& dma, Pinout pinout, unsigned baud,
           hal::UartParity   parity    = hal::UartParity::None,
           hal::UartStopBits stop_bits = hal::UartStopBits::One)
    requires(FC == hal::UartFlowControl::None
             && OM == hal::UartOperatingMode::Dma)
      : huart{} {
    static_assert(dma.template ChannelEnabled<TxDmaChannel>(),
                  "TX DMA channel must be enabled");
    static_assert(dma.template ChannelEnabled<RxDmaChannel>(),
                  "RX DMA channel must be enabled");

    // Set up tx and rx pins
    Pin::InitializeAlternate(
        pinout.tx, hal::FindPinAFMapping(UartTxPinMappings, Id, pinout.tx)->af,
        pinout.pull_tx);
    Pin::InitializeAlternate(
        pinout.rx, hal::FindPinAFMapping(UartRxPinMappings, Id, pinout.rx)->af,
        pinout.pull_rx);

    // Initialize UART
    detail::SetupUartNoFc(Id, huart, baud, parity, stop_bits);

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

 public:
  constexpr void HandleInterrupt() noexcept {}

 protected:
  void ReceiveComplete(std::size_t n_bytes) noexcept {}

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