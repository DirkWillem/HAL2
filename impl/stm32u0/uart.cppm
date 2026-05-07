module;

#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <stm32u0xx_hal.h>

#include <internal/peripheral_availability.h>

export module hal.stm32u0:uart;

import hstd;
import hal.abstract;

import :dma;
import :peripherals;
import :pin_mapping.uart;
import :nvic;

extern "C" {

[[maybe_unused]] void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size);

[[maybe_unused]] void HAL_UART_TxCpltCallback(UART_HandleTypeDef*);
}

namespace stm32u0 {

/** UART operating mode. */
export enum class UartOperatingMode {
  Poll,        //!< Operate using blocking polling loop
  Interrupt,   //!< Transmit/receive bytes in interrupt
  Dma,         //!< Transmit using DMA
};

/** UART hardware flow control setting. */
export enum class UartFlowControl {
  None,    //!< No flow control
  Rs485,   //!< RS-485 flow control
};

/** RS-485 Drive enable polarity. */
export enum class Rs485DriveEnablePolarity : uint32_t {
  High = UART_DE_POLARITY_HIGH,   //!< High polarity
  Low  = UART_DE_POLARITY_LOW     //!< Low polarity
};

/** UART peripheral configuration settings.*/
export struct UartConfig {
  hal::UartParity   parity    = hal::UartParity::None;    //!< UART parity
  hal::UartStopBits stop_bits = hal::UartStopBits::One;   //!< UART stop bits

  UartFlowControl          flow_control = UartFlowControl::None;   //!< Hardware flow control
  Rs485DriveEnablePolarity de_polarity =
      Rs485DriveEnablePolarity::High;   //!< RS-485 Drive Enable polarity
  uint8_t rs485_assertion_time   = 0;   //!< RS-485 Drive Enable assertion time
  uint8_t rs485_deassertion_time = 0;   //!< RS-485 Drive Enable de-assertion time

  /**
   * @brief Validates the UART settings, and produces (compile time) UB if the
   * configuration is invalid.
   *
   * @return Always returns \c true, to enable use in \c static_assert.
   */
  [[nodiscard]] consteval bool Validate() const noexcept {
    // Validate RS-485-related parameters
    hstd::Assert(rs485_assertion_time <= 31, "RS-485 assertion time must be between 0 and 31");
    hstd::Assert(rs485_deassertion_time <= 31, "RS-485 assertion time must be between 0 and 31");

    return true;
  }
};

template <UartId Id, UartConfig C>
struct UartPinoutHelper;

/**
 * UART pinout when no flow control is enabled.
 * @tparam Id UART instance ID
 * @tparam C UART configuration
 */
template <UartId Id, UartConfig C>
  requires(C.flow_control == UartFlowControl::None)
struct UartPinoutHelper<Id, C> {
  struct Pinout {
    /**
     * @brief Constructor
     *
     * @param tx TX pin.
     * @param rx RX pin.
     * @param pull_tx Pull-up/pull-down configuration for \c tx.
     * @param pull_rx Pull-up/pull-down configuration for \c rx.
     */
    consteval Pinout(PinId tx, PinId rx, hal::PinPull pull_tx = hal::PinPull::NoPull,
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

    /**
     * @brief Configures the pins in the pinout for use as a UART peripheral.
     */
    void Initialize() const noexcept {
      Pin::InitializeAlternate(tx, tx_af, pull_tx);
      Pin::InitializeAlternate(rx, rx_af, pull_rx);
    }

    PinId tx;   //!< TX pin
    PinId rx;   //!< RX pin

    hal::PinPull pull_tx;   //!< Pull-up/pull-down configuration for \c tx
    hal::PinPull pull_rx;   //!< Pull-up/pull-down configuration for \c rx

    unsigned tx_af{0};   //!< Alternate function number for \c tx
    unsigned rx_af{0};   //!< Alternate function number for \c rx
  };
};

/**
 * UART pinout when RS-485 flow control is enabled.
 * @tparam Id UART instance ID
 * @tparam C UART configuration
 */
template <UartId Id, UartConfig C>
  requires(C.flow_control == UartFlowControl::Rs485)
struct UartPinoutHelper<Id, C> {
  struct Pinout {
    /**
     * @brief Constructor
     *
     * @param tx TX pin.
     * @param rx RX pin.
     * @param de RS-485 Drive Enable pin.
     * @param pull_tx Pull-up/pull-down configuration for \c tx.
     * @param pull_rx Pull-up/pull-down configuration for \c rx.
     * @param pull_de Pull-up/pull-down configuration for \c de.
     */
    consteval Pinout(PinId tx, PinId rx, PinId de, hal::PinPull pull_tx = hal::PinPull::NoPull,
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
      hstd::Assert(hal::FindPinAFMapping(UartRtsPinMappings, Id, de).has_value(),
                   "DE pin must be valid");
    }

    /**
     * @brief Configures the pins in the pinout for use as a UART peripheral.
     */
    void Initialize() const noexcept {
      Pin::InitializeAlternate(tx, hal::FindPinAFMapping(UartTxPinMappings, Id, tx)->af, pull_tx);
      Pin::InitializeAlternate(rx, hal::FindPinAFMapping(UartRxPinMappings, Id, rx)->af, pull_rx);
      Pin::InitializeAlternate(de, hal::FindPinAFMapping(UartRtsPinMappings, Id, de)->af, pull_de);
    }

    PinId tx;   //!< TX pin
    PinId rx;   //!< RX pin
    PinId de;   //!< DE pin

    hal::PinPull pull_tx;   //!< Pull-up/pull-down configuration for \c tx
    hal::PinPull pull_rx;   //!< Pull-up/pull-down configuration for \c rx
    hal::PinPull pull_de;   //!< Pull-up/pull-down configuration for \c de
  };
};

constexpr void EnableUartClk(UartId id) noexcept {
  using enum UartId;

  switch (id) {
#ifdef HAS_USART_1_2_3_4
  case Usart1: __HAL_RCC_USART1_CLK_ENABLE();
  case Usart2: __HAL_RCC_USART2_CLK_ENABLE();
  case Usart3: __HAL_RCC_USART3_CLK_ENABLE();
  case Usart4: __HAL_RCC_USART4_CLK_ENABLE();
#endif
#ifdef HAS_LPUART_1_2
  case LpUart1: __HAL_RCC_LPUART1_CLK_ENABLE();
  case LpUart2: __HAL_RCC_LPUART2_CLK_ENABLE();
#endif
#ifdef HAS_LPUART_3
  case LpUart3: __HAL_RCC_LPUART3_CLK_ENABLE();
#endif
  default: break;
  }
}

[[nodiscard]] constexpr IRQn_Type GetIrqn(UartId id) noexcept {
  using enum UartId;
  switch (id) {
#if defined(HAS_USART_1_2_3_4) && defined(HAS_LPUART_1_2)
  case Usart1: return USART1_IRQn;
  case Usart2: [[fallthrough]];
  case LpUart2: return USART2_LPUART2_IRQn;
  case Usart3: [[fallthrough]];
  case LpUart1: return USART3_LPUART1_IRQn;
#endif
#if defined(HAS_USART_1_2_3_4) && defined(HAS_LPUART_3)
  case Usart4: [[fallthrough]];
  case LpUart3: return USART4_LPUART3_IRQn;
#else
#error "Not implemented: LPUART 3 not available."
#endif
  default: std::unreachable();
  }
}

[[nodiscard]] constexpr uint32_t ToHalStopBits(hal::UartStopBits stop_bits) noexcept {
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

  HAL_RS485Ex_Init(&huart, static_cast<uint32_t>(cfg.de_polarity), cfg.rs485_assertion_time,
                   cfg.rs485_deassertion_time);
}

void InitializeUartForPollMode(UART_HandleTypeDef& huart) noexcept {
  HAL_UARTEx_SetTxFifoThreshold(&huart, UART_TXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_SetRxFifoThreshold(&huart, UART_RXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_DisableFifoMode(&huart);
}

/**
 * UART Transmit DMA channel.
 * @tparam Id UART Id.
 * @tparam Prio DMA Priority.
 */
export template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartTxDma = DmaChannel<Id, UartDmaRequest::Tx, Prio>;

/**
 * UART Receive DMA channel.
 * @tparam Id UART Id.
 * @tparam Prio DMA Priority.
 */
export template <UartId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using UartRxDma = DmaChannel<Id, UartDmaRequest::Rx, Prio>;

/**
 * Implementation for UART.
 * @tparam Id UART Id.
 * @tparam OM UART Operating Mode.
 * @tparam C UART configuration.
 */
export template <typename Impl, UartId Id, UartOperatingMode OM = UartOperatingMode::Poll,
                 UartConfig C = {}, typename... Rest>
class UartImpl : public hal::UsedPeripheral {
  friend void ::HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size);
  friend void ::HAL_UART_TxCpltCallback(UART_HandleTypeDef*);

  static_assert(C.Validate());

 public:
  UartImpl(const UartImpl&)            = delete;
  UartImpl(UartImpl&&)                 = delete;
  UartImpl& operator=(const UartImpl&) = delete;
  UartImpl& operator=(UartImpl&&)      = delete;
  ~UartImpl()                          = default;

  static constexpr auto OperatingMode = OM;
  using TxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Tx>;
  using RxDmaChannel                  = DmaChannel<Id, UartDmaRequest::Rx>;
  using Pinout                        = typename UartPinoutHelper<Id, C>::Pinout;

  void HandleInterrupt() noexcept { HAL_UART_IRQHandler(&huart); }

  /**
   * @brief Callback that is invoked when a receive completes.
   * @param n_bytes Number of received bytes.
   */
  void ReceiveComplete(std::size_t n_bytes) noexcept {
    if constexpr (hal::AsyncUart<Impl>) {
      static_cast<Impl*>(this)->UartReceiveCallback(rx_buf.subspan(0, n_bytes));
    }
  }

  /**
   * @brief Callback that is invoked when a transmit completex.
   */
  void TransmitComplete() noexcept {
    if constexpr (hal::AsyncUart<Impl>) {
      static_cast<Impl*>(this)->UartTransmitCallback();
    }
  }

  /**
   * @brief Writes a string to the UART when in Poll mode.
   * @param sv View of the string to write.
   */
  void Write(std::string_view sv)
    requires(OM == UartOperatingMode::Poll)
  {
    HAL_UART_Transmit(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(), sv.size(), 500);
  }

  /**
   * @brief Writes a string to the UART when in Interrupt mode.
   * @param sv View of the string to write.
   */
  void Write(std::string_view sv)
    requires(OM == UartOperatingMode::Interrupt)
  {
    HAL_UART_Transmit_IT(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(), sv.size());
  }

  /**
   * @brief Writes a string to the UART when in DMA mode.
   * @param sv View of the string to write.
   */
  void Write(std::string_view sv)
    requires(OM == UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, hstd::ReinterpretSpan<uint8_t>(sv).data(), sv.size());
  }

  /**
   * @brief Writes a string of bytes to the UART when in DMA mode.
   * @param data Bytes to write.
   */
  void Write(std::span<const std::byte> data)
    requires(OM == UartOperatingMode::Dma)
  {
    HAL_UART_Transmit_DMA(&huart, hstd::ReinterpretSpan<uint8_t>(data).data(), data.size());
  }

  /**
   * @brief Receives a single packet of data into the given buffer in DMA mode.
   * @param into Receive buffer.
   */
  void Receive(std::span<std::byte> into) noexcept
    requires(OM == UartOperatingMode::Dma)
  {
    rx_buf = into;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart, hstd::ReinterpretSpanMut<uint8_t>(rx_buf).data(),
                                 rx_buf.size());
  }

  /**
   * @brief Singleton constructor.
   *
   * @return Singleton instance
   */
  [[nodiscard]] static Impl& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  UART_HandleTypeDef huart;   //!< UART handle.

 protected:
  /**
   * @brief Constructor for UART without flow control in \c Poll or \c Interrupt
   * mode.
   *
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

  /**
   * @brief Constructor for UART without flow control in \c Dma or \c DmaRtos
   * mode.
   *
   * @param pinout UART pinout
   * @param baud UART baud rate
   */
  UartImpl(hal::Dma auto& dma, Pinout pinout, unsigned baud)
    requires(OM == UartOperatingMode::Dma)
      : huart{} {
    using Dma = std::decay_t<decltype(dma)>;
    static_assert(Dma::template ChannelEnabled<TxDmaChannel>(), "TX DMA channel must be enabled");
    static_assert(Dma::template ChannelEnabled<RxDmaChannel>(), "RX DMA channel must be enabled");

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
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal, hal::DmaDataWidth::Byte, false,
        hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&huart, hdmatx, htxdma);

    auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
        hal::DmaDirection::PeriphToMem, hal::DmaMode::Normal, hal::DmaDataWidth::Byte, false,
        hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&huart, hdmarx, hrxdma);

    EnableInterrupt<GetIrqn(Id), Impl>();
  }

  std::span<std::byte> rx_buf{};
};

/**
 * Marker class for UART peripherals
 * @tparam Id UART id
 */
export template <UartId Id>
class Uart : public hal::UnusedPeripheral<Uart<Id>> {
 public:
  constexpr void HandleInterrupt() noexcept {}
  void           ReceiveComplete(std::size_t) noexcept {}
  void           TransmitComplete() noexcept {}

  UART_HandleTypeDef huart{};
};

#ifdef HAS_USART_1_2_3_4
export using Usart1 = Uart<UartId::Usart1>;
export using Usart2 = Uart<UartId::Usart2>;
export using Usart3 = Uart<UartId::Usart3>;
export using Usart4 = Uart<UartId::Usart4>;
#endif
#ifdef HAS_LPUART_1_2
export using LpUart1 = Uart<UartId::LpUart1>;
export using LpUart2 = Uart<UartId::LpUart2>;
#endif
#ifdef HAS_LPUART_3
export using LpUart3 = Uart<UartId::LpUart3>;
#endif

}   // namespace stm32u0