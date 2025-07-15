module;

#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <stm32g4xx_hal.h>

export module hal.stm32g4:uart;

import hstd;
import hal.abstract;

import rtos.concepts;

import :dma;
import :peripherals;
import :pin_mapping.uart;
import :nvic;

extern "C" {

[[maybe_unused]] void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart,
                                                 uint16_t            size);

[[maybe_unused]] void HAL_UART_TxCpltCallback(UART_HandleTypeDef*);
}

namespace stm32g4 {

export enum class UartOperatingMode { Poll, Interrupt, Dma, DmaRtos };

export enum class UartFlowControl {
  None,
  Rs485,
};

export enum class Rs485DriveEnablePolarity : uint32_t {
  High = UART_DE_POLARITY_HIGH,
  Low  = UART_DE_POLARITY_LOW
};

/**
 * Struct containing all configuration parameters of a UART peripheral
 */
export struct UartConfig {
  hal::UartParity   parity    = hal::UartParity::None;
  hal::UartStopBits stop_bits = hal::UartStopBits::One;

  UartFlowControl          flow_control = UartFlowControl::None;
  Rs485DriveEnablePolarity de_polarity  = Rs485DriveEnablePolarity::High;
  uint8_t                  rs485_assertion_time   = 0;
  uint8_t                  rs485_deassertion_time = 0;

  [[nodiscard]] consteval bool Validate() const noexcept {
    // Validate RS-485-related parameters
    hstd::Assert(rs485_assertion_time <= 31,
                 "RS-485 assertion time must be between 0 and 31");
    hstd::Assert(rs485_deassertion_time <= 31,
                 "RS-485 assertion time must be between 0 and 31");

    return true;
  }
};

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
      hstd::Assert(hal::FindPinAFMapping(UartTxPinMappings, Id, tx).has_value(),
                   "TX pin must be valid");
      hstd::Assert(hal::FindPinAFMapping(UartRxPinMappings, Id, rx).has_value(),
                   "RX pin must be valid");

      tx_af = hal::FindPinAFMapping(UartTxPinMappings, Id, tx)->af;
      rx_af = hal::FindPinAFMapping(UartRxPinMappings, Id, rx)->af;
    }

    void Initialize() const noexcept {
      Pin::InitializeAlternate(tx, tx_af, pull_tx);
      Pin::InitializeAlternate(rx, rx_af, pull_rx);
    }

    PinId tx;
    PinId rx;

    hal::PinPull pull_tx;
    hal::PinPull pull_rx;

    unsigned tx_af{0};
    unsigned rx_af{0};
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
      hstd::Assert(hal::FindPinAFMapping(UartTxPinMappings, Id, tx).has_value(),
                   "TX pin must be valid");
      hstd::Assert(hal::FindPinAFMapping(UartRxPinMappings, Id, rx).has_value(),
                   "RX pin must be valid");
      hstd::Assert(
          hal::FindPinAFMapping(UartRtsPinMappings, Id, de).has_value(),
          "DE pin must be valid");
    }

    void Initialize() const noexcept {
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

constexpr void EnableUartClk(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: __HAL_RCC_USART1_CLK_ENABLE(); break;
  case UartId::Usart2: __HAL_RCC_USART2_CLK_ENABLE(); break;
  case UartId::Usart3: __HAL_RCC_USART3_CLK_ENABLE(); break;
  case UartId::Uart4: __HAL_RCC_UART4_CLK_ENABLE(); break;
  case UartId::Uart5: __HAL_RCC_UART5_CLK_ENABLE(); break;
  case UartId::LpUart1: __HAL_RCC_LPUART1_CLK_ENABLE(); break;
  }
}

[[nodiscard]] constexpr IRQn_Type GetIrqn(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: return USART1_IRQn;
  case UartId::Usart2: return USART2_IRQn;
  case UartId::Usart3: return USART3_IRQn;
  case UartId::Uart4: return UART4_IRQn;
  case UartId::Uart5: return UART5_IRQn;
  case UartId::LpUart1: return LPUART1_IRQn;
  }

  std::unreachable();
}
[[nodiscard]] constexpr uint32_t
ToHalStopBits(hal::UartStopBits stop_bits) noexcept {
  switch (stop_bits) {
  case hal::UartStopBits::Half: return UART_STOPBITS_0_5;
  case hal::UartStopBits::One: return UART_STOPBITS_1;
  case hal::UartStopBits::OneAndHalf: return UART_STOPBITS_1_5;
  case hal::UartStopBits::Two: return UART_STOPBITS_2;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t ToHalParity(hal::UartParity parity) {
  switch (parity) {
  case hal::UartParity::Even: return USART_PARITY_EVEN;
  case hal::UartParity::Odd: return USART_PARITY_ODD;
  case hal::UartParity::None: return USART_PARITY_NONE;
  }

  std::unreachable();
}

void SetupUartNoFc(UartId id, UART_HandleTypeDef& huart, unsigned baud,
                   const UartConfig& cfg) noexcept {
  // Enable UART clock
  EnableUartClk(id);

  // Set up handle
  huart.Instance = GetUartPointer(id);
  huart.Init     = {
          .BaudRate       = baud,
          .WordLength     = USART_WORDLENGTH_8B,
          .StopBits       = ToHalStopBits(cfg.stop_bits),
          .Parity         = ToHalParity(cfg.parity),
          .Mode           = USART_MODE_TX_RX,
          .HwFlowCtl      = UART_HWCONTROL_NONE,
          .OverSampling   = UART_OVERSAMPLING_16,
          .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
          .ClockPrescaler = UART_PRESCALER_DIV1,

  };
  HAL_UART_Init(&huart);
}

void SetupUartRs485(UartId id, UART_HandleTypeDef& huart, unsigned baud,
                    const UartConfig& cfg) noexcept {
  // Enable UART clock
  EnableUartClk(id);

  // Set up handle
  huart.Instance = GetUartPointer(id);
  huart.Init     = {
          .BaudRate       = baud,
          .WordLength     = USART_WORDLENGTH_8B,
          .StopBits       = ToHalStopBits(cfg.stop_bits),
          .Parity         = ToHalParity(cfg.parity),
          .Mode           = USART_MODE_TX_RX,
          .HwFlowCtl      = UART_HWCONTROL_NONE,
          .OverSampling   = UART_OVERSAMPLING_16,
          .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
          .ClockPrescaler = UART_PRESCALER_DIV1,

  };

  HAL_RS485Ex_Init(&huart, static_cast<uint32_t>(cfg.de_polarity),
                   cfg.rs485_assertion_time, cfg.rs485_deassertion_time);
}

void InitializeUartForPollMode(UART_HandleTypeDef& huart) noexcept {
  HAL_UARTEx_SetTxFifoThreshold(&huart, UART_TXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_SetRxFifoThreshold(&huart, UART_RXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_DisableFifoMode(&huart);
}

/**
 * UART Transmit DMA channel
 * @tparam Id UART Id
 * @tparam Prio DMA Priority
 */
export template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartTxDma = DmaChannel<Id, UartDmaRequest::Tx, Prio>;

/**
 * UART Receive DMA channel
 * @tparam Id UART Id
 * @tparam Prio DMA Priority
 */
export template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartRxDma = DmaChannel<Id, UartDmaRequest::Rx, Prio>;

template <UartOperatingMode OM, typename... Ts>
class UartImplBase {
 protected:
  using EG = int;
};

template <typename OS>
class UartImplBase<UartOperatingMode::DmaRtos, OS> {
 protected:
  using EG                            = typename OS::EventGroup;
  static constexpr uint32_t TxDoneBit = (0b1U << 0U);
  static constexpr uint32_t RxDoneBit = (0b1U << 1U);

  EG                        event_group{};
  std::tuple<EG*, uint32_t> tx_event_group{};
  std::tuple<EG*, uint32_t> rx_event_group{};

  std::optional<std::span<std::byte>> rx_data{};
};

/**
 * Implementation for UART
 * @tparam Id UART Id
 * @tparam FC UART Flow Control
 * @tparam OM UART Operating Mode
 */
export template <typename Impl, UartId Id,
                 UartOperatingMode OM = UartOperatingMode::Poll,
                 UartConfig        C  = {}, typename... Rest>
class UartImpl
    : public hal::UsedPeripheral
    , UartImplBase<OM, Rest...> {
  friend void ::HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart,
                                           uint16_t            size);
  friend void ::HAL_UART_TxCpltCallback(UART_HandleTypeDef*);

  static_assert(C.Validate());

 public:
  static constexpr auto OperatingMode = OM;
  using TxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Tx>;
  using RxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Rx>;
  using Pinout = typename UartPinoutHelper<Id, C>::Pinout;

  void HandleInterrupt() noexcept { HAL_UART_IRQHandler(&huart); }

  /**
   * Writes a string to the UART
   * @param sv View of the string to write
   */
  void Write(std::string_view sv)
    requires(OM == UartOperatingMode::Poll)
  {
    HAL_UART_Transmit(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(),
                      sv.size(), 500);
  }

  void Write(std::string_view sv)
    requires(OM == UartOperatingMode::Interrupt)
  {
    HAL_UART_Transmit_IT(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(),
                         sv.size());
  }

  void Write(std::string_view sv)
    requires(OM == UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(),
                          sv.size());
  }

  void Write(std::span<const std::byte> data)
    requires(OM == UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, hstd::ReinterpretSpan<uint8_t>(data).data(),
                          data.size());
  }

  /**
   * Writes a string over the UART and blocks until transmitted
   * @param sv String to write
   * @param timeout Maximum write timeout
   * @return Whether the write was successful
   */
  bool Write(std::string_view sv, hstd::Duration auto timeout)
    requires(OM == UartOperatingMode::DmaRtos)
  {
    Write(sv, UartImplBase<OM, Rest...>::event_group,
          UartImplBase<OM, Rest...>::TxDoneBit);

    return UartImplBase<OM, Rest...>::event_group
        .Wait(UartImplBase<OM, Rest...>::TxDoneBit, timeout)
        .has_value();
  }

  /**
   * Writes bytes to the UART and blocks until transmitted
   * @param data Data to write
   * @param timeout Write timeout
   * @return Whether the write was successful
   */
  bool Write(std::span<const std::byte> data, hstd::Duration auto timeout)
    requires(OM == UartOperatingMode::DmaRtos)
  {
    Write(data, UartImplBase<OM, Rest...>::event_group,
          UartImplBase<OM, Rest...>::TxDoneBit);

    return UartImplBase<OM, Rest...>::event_group
        .Wait(UartImplBase<OM, Rest...>::TxDoneBit, timeout)
        .has_value();
  }

  /**
   * Writes a string to the UART and sets a bit in the event group on
   * transmission
   * @param sv String to write
   * @param event_group Event group to set a bit in when write completes
   * @param bitmask Bit to set in the event group
   */
  void Write(std::string_view                        sv,
             typename UartImplBase<OM, Rest...>::EG& event_group,
             uint32_t                                bitmask)
    requires(OM == UartOperatingMode::DmaRtos)
  {
    UartImplBase<OM, Rest...>::tx_event_group =
        std::make_pair(&event_group, bitmask);

    HAL_UART_Transmit_DMA(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(),
                          sv.size());
  }

  /**
   * Writes bytes to the UART and sets a bit in the event group on
   * transmission
   * @param data Data to write
   * @param event_group Event group to set a bit in when write completes
   * @param bitmask Bit to set in the event group
   */
  void Write(std::span<const std::byte>              data,
             typename UartImplBase<OM, Rest...>::EG& event_group,
             uint32_t                                bitmask)
    requires(OM == UartOperatingMode::DmaRtos)
  {
    UartImplBase<OM, Rest...>::tx_event_group =
        std::make_pair(&event_group, bitmask);

    HAL_UART_Transmit_DMA(&huart, hstd::ReinterpretSpan<uint8_t>(data).data(),
                          data.size());
  }

  void Receive(std::span<std::byte> into) noexcept
    requires(OM == UartOperatingMode::Dma)
  {
    rx_buf = into;
    HAL_UARTEx_ReceiveToIdle_DMA(
        &huart, hstd::ReinterpretSpanMut<uint8_t>(rx_buf).data(),
        rx_buf.size());
  }

  /**
   * Starts receiving data from the UART and blocks until data is received
   * @param into Buffer to receive data into
   * @param timeout Receive timeout
   * @return Received data if data is received within time window, std::nullopt
   * otherwise
   */
  std::optional<std::span<std::byte>>
  Receive(std::span<std::byte> into, hstd::Duration auto timeout) noexcept
    requires(OM == UartOperatingMode::DmaRtos)
  {
    Receive(into, UartImplBase<OM, Rest...>::event_group,
            UartImplBase<OM, Rest...>::RxDoneBit);

    if (UartImplBase<OM, Rest...>::event_group
            .Wait(UartImplBase<OM, Rest...>::RxDoneBit, timeout)
            .has_value()) {
      return UartImplBase<OM, Rest...>::rx_data;
    } else {
      return {};
    }
  }

  /**
   * Starts receiving data from the UART into the provided buffer and sets a bit
   * in the provided event group upon data receival
   * @param into Buffer to receive data into
   * @param event_group Event group in which to set a bit upon data receival
   * @param bitmask Bit to set in the event group
   */
  void Receive(std::span<std::byte>                    into,
               typename UartImplBase<OM, Rest...>::EG& event_group,
               uint32_t                                bitmask) noexcept
    requires(OM == UartOperatingMode::DmaRtos)
  {
    UartImplBase<OM, Rest...>::rx_data = std::nullopt;
    UartImplBase<OM, Rest...>::rx_event_group =
        std::make_pair(&event_group, bitmask);
    rx_buf = into;
    HAL_UARTEx_ReceiveToIdle_DMA(
        &huart, hstd::ReinterpretSpanMut<uint8_t>(rx_buf).data(),
        rx_buf.size());
  }

  /**
   * Returns the received data using the Receive method with an explicit event
   * group
   * @return Optional containing received data if data was received, or
   * std::nullopt if no data was received
   */
  std::optional<std::span<std::byte>> ReceivedData() noexcept
    requires(OM == UartOperatingMode::DmaRtos)
  {
    return UartImplBase<OM, Rest...>::rx_data;
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
    requires(OM != UartOperatingMode::Dma)
      : huart{} {
    // Set up pins
    pinout.Initialize();

    // Initialize UART
    if constexpr (C.flow_control == UartFlowControl::None) {
      SetupUartNoFc(Id, huart, baud, C);
    } else if constexpr (C.flow_control == UartFlowControl::Rs485) {
      SetupUartRs485(Id, huart, baud, C);
    }

    // Set up UART for the requested operation mode
    if constexpr (OM == UartOperatingMode::Poll) {
      InitializeUartForPollMode(huart);
    } else if constexpr (OM == UartOperatingMode::Interrupt) {
      EnableInterrupt<GetIrqn(Id), Impl>();
    } else {
      std::unreachable();
    }
  }

  void ReceiveComplete(std::size_t n_bytes) noexcept {
    if constexpr (OM != UartOperatingMode::DmaRtos) {
      static_cast<Impl*>(this)->UartReceiveCallback(rx_buf.subspan(0, n_bytes));
    } else {
      UartImplBase<OM, Rest...>::rx_data = rx_buf.subspan(0, n_bytes);

      auto& [eg, bitmask] = UartImplBase<OM, Rest...>::rx_event_group;
      eg->SetBitsFromInterrupt(bitmask);
    }
  }

  void TransmitComplete() noexcept {
    if constexpr (OM != UartOperatingMode::DmaRtos) {
      if constexpr (hal::AsyncUart<Impl>) {
        static_cast<Impl*>(this)->UartTransmitCallback();
      }
    } else {
      auto& [eg, bitmask] = UartImplBase<OM, Rest...>::tx_event_group;
      eg->SetBitsFromInterrupt(bitmask);
    }
  }

  /**
   * Constructor for UART without flow control in DMA mode
   * @param pinout UART pinout
   * @param baud UART baud rate
   */
  UartImpl(hal::Dma auto& dma, Pinout pinout, unsigned baud)
    requires(OM == UartOperatingMode::Dma || OM == UartOperatingMode::DmaRtos)
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
      SetupUartNoFc(Id, huart, baud, C);
    } else if constexpr (C.flow_control == UartFlowControl::Rs485) {
      SetupUartRs485(Id, huart, baud, C);
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

    EnableInterrupt<GetIrqn(Id), Impl>();
  }

  std::span<std::byte> rx_buf{};
  UART_HandleTypeDef   huart;
};

/**
 * Marker class for UART peripherals
 * @tparam Id UART id
 */
export template <UartId Id>
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

export using Usart1  = Uart<UartId::Usart1>;
export using Usart2  = Uart<UartId::Usart2>;
export using Usart3  = Uart<UartId::Usart3>;
export using Uart4   = Uart<UartId::Uart4>;
export using Uart5   = Uart<UartId::Uart5>;
export using LpUart1 = Uart<UartId::LpUart1>;

}   // namespace stm32g4