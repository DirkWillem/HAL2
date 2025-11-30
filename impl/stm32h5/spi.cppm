module;

#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <stm32h5xx.h>
#include <stm32h5xx_hal_conf.h>
#include <stm32h5xx_hal_rcc.h>
#include <stm32h5xx_hal_spi.h>

export module hal.stm32h5:spi;

import hal.abstract;
import rtos.concepts;

export import :spi.config;
export import :spi.pinout;

import :clocks;
import :dma;
import :peripherals;
import :pin;
import :nvic;

extern "C" {

[[maybe_unused]] void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);
[[maybe_unused]] void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef*);
}

namespace stm32h5 {

enum class SpiBaudPrescaler : uint32_t {
  Prescale2   = SPI_BAUDRATEPRESCALER_2,
  Prescale4   = SPI_BAUDRATEPRESCALER_4,
  Prescale8   = SPI_BAUDRATEPRESCALER_8,
  Prescale16  = SPI_BAUDRATEPRESCALER_16,
  Prescale32  = SPI_BAUDRATEPRESCALER_32,
  Prescale64  = SPI_BAUDRATEPRESCALER_64,
  Prescale128 = SPI_BAUDRATEPRESCALER_128,
  Prescale256 = SPI_BAUDRATEPRESCALER_256,
};

[[nodiscard]] constexpr IRQn_Type GetIrqn(SpiId id) noexcept {
  using enum SpiId;
  switch (id) {
  case Spi1: return SPI1_IRQn;
  case Spi2: return SPI2_IRQn;
  case Spi3: return SPI3_IRQn;
  case Spi4: return SPI4_IRQn;
  default: std::unreachable();
  }
}

[[nodiscard]] constexpr uint32_t
ToHalMasterDirection(const hal::SpiTransmissionType tt) noexcept {
  switch (tt) {
  case hal::SpiTransmissionType::FullDuplex: return SPI_DIRECTION_2LINES;
  case hal::SpiTransmissionType::HalfDuplex: return SPI_DIRECTION_1LINE;
  case hal::SpiTransmissionType::TxOnly: return SPI_DIRECTION_2LINES;
  case hal::SpiTransmissionType::RxOnly: return SPI_DIRECTION_2LINES_RXONLY;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t ToHalDataSize(unsigned size) noexcept {
  // ReSharper disable once CppRedundantParentheses
  return (size - 1);
}

/**
 * @brief SPI Transmit (TX) DMA channel.
 *
 * @tparam Id SPI Id
 * @tparam Prio DMA channel priority.
 */
export template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<Id, SpiDmaRequest::Tx, Prio>;

/**
 * @brief SPI Receive (RX) DMA channel.
 *
 * @tparam Id SPI Id
 * @tparam Prio DMA channel priority.
 */
export template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<Id, SpiDmaRequest::Rx, Prio>;

template <SpiOperatingMode OM, typename... Ts>
class SpiImplBase {
 protected:
  using EG = int;
};

template <typename OS>
class SpiImplBase<SpiOperatingMode::Dma, OS> {
 protected:
  using EG = int;

  DMA_HandleTypeDef* hdma_tx{};
  DMA_HandleTypeDef* hdma_rx{};
};

template <typename OS>
class SpiImplBase<SpiOperatingMode::DmaRtos, OS> {
 protected:
  using EG                            = typename OS::EventGroup;
  static constexpr uint32_t TxDoneBit = (0b1U << 0U);
  static constexpr uint32_t RxDoneBit = (0b1U << 1U);

  EG                        event_group{};
  std::tuple<EG*, uint32_t> tx_event_group{};
  std::tuple<EG*, uint32_t> rx_event_group{};

  DMA_HandleTypeDef* hdma_tx{};
  DMA_HandleTypeDef* hdma_rx{};
};

export template <SpiId Id>
void EnableSpiClock() noexcept {
  using enum SpiId;

  if constexpr (Id == Spi1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    return;
  }
  if constexpr (Id == Spi2) {
    __HAL_RCC_SPI2_CLK_ENABLE();
    return;
  }
  if constexpr (Id == Spi3) {
    __HAL_RCC_SPI3_CLK_ENABLE();
    return;
  }
#ifdef SPI4
  if constexpr (Id == Spi4) {
    __HAL_RCC_SPI4_CLK_ENABLE();
    return;
  }
#endif
}

export template <SpiId Id, SpiSourceClock SC>
bool SetupSourceClock() {
  using enum SpiSourceClock;

  if constexpr (Id == SpiId::Spi1) {
    RCC_PeriphCLKInitTypeDef init = {
        .PeriphClockSelection = RCC_PERIPHCLK_SPI1,
    };

    hstd::Unimplemented(SC == AudioClk || SC == Per,
                        "AUDIOCLK en PER source clocks");

    switch (SC) {
    case Pll1Q: init.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL1Q; break;
    case Pll2P: init.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL2P; break;
    case Pll3P: init.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL3P; break;
    default: break;
    }

    return HAL_RCCEx_PeriphCLKConfig(&init) == HAL_OK;
  }

  if constexpr (Id == SpiId::Spi2) {
    RCC_PeriphCLKInitTypeDef init = {
        .PeriphClockSelection = RCC_PERIPHCLK_SPI2,
    };

    hstd::Unimplemented(SC == AudioClk || SC == Per,
                        "AUDIOCLK en PER source clocks");

    switch (SC) {
    case Pll1Q: init.Spi2ClockSelection = RCC_SPI2CLKSOURCE_PLL1Q; break;
    case Pll2P: init.Spi2ClockSelection = RCC_SPI2CLKSOURCE_PLL2P; break;
    case Pll3P: init.Spi2ClockSelection = RCC_SPI2CLKSOURCE_PLL3P; break;
    default: break;
    }

    return HAL_RCCEx_PeriphCLKConfig(&init) == HAL_OK;
  }

  if constexpr (Id == SpiId::Spi3) {
    RCC_PeriphCLKInitTypeDef init = {
        .PeriphClockSelection = RCC_PERIPHCLK_SPI3,
    };

    hstd::Unimplemented(SC == AudioClk || SC == Per,
                        "AUDIOCLK en PER source clocks");

    switch (SC) {
    case Pll1Q: init.Spi3ClockSelection = RCC_SPI3CLKSOURCE_PLL1Q; break;
    case Pll2P: init.Spi3ClockSelection = RCC_SPI3CLKSOURCE_PLL2P; break;
    case Pll3P: init.Spi3ClockSelection = RCC_SPI3CLKSOURCE_PLL3P; break;
    default: break;
    }

    return HAL_RCCEx_PeriphCLKConfig(&init) == HAL_OK;
  }

  std::unreachable();
}

/**
 * @brief Enables the SPI interrupt.
 * @tparam Id ID of the SPI to enable the interrupt of.
 * @tparam Impl Peripheral implementation type.
 */
export template <SpiId Id, typename Impl>
void EnableSpiInterrupt() noexcept {
  EnableInterrupt<GetIrqn(Id), Impl>();
}

template <typename T, SpiSettings SS>
concept SpiData =
    std::is_same_v<std::remove_cvref_t<T>,
                   std::conditional_t<(SS.data_size > 8), uint16_t, uint8_t>>
    || (SS.data_size == 8 && std::is_same_v<std::remove_cvref_t<T>, std::byte>);

/**
 * @brief SPI peripheral implementation.
 *
 * @tparam Impl Implementing type, using CRTP pattern.
 * @tparam Id SPI ID.
 * @tparam CS Clock Settings.
 * @tparam SS SPI Settings.
 * @tparam Rest Rest arguments, dependent on operating mode. For \c DmaRtos
 * mode, this must be the RTOS type.
 */
export template <typename Impl, SpiId Id, ClockSettings CS, SpiSettings SS,
                 typename... Rest>
class SpiImpl
    : public hal::UsedPeripheral
    , SpiImplBase<SS.operating_mode, Rest...> {
  using PinoutHelper = SpiPinoutHelper<Id, SS>;

  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);
  friend void ::HAL_SPI_TxCpltCallback(SPI_HandleTypeDef*);

 public:
  static constexpr auto Mode = SS.mode;   //!< SPI mode.
  static constexpr auto TransmissionType =
      SS.transmission_type;                        //!< SPI transmission type.
  static constexpr auto DataSize = SS.data_size;   //!< SPI data size.

  /** Pinout type */
  using Pinout = PinoutHelper::Pinout;

  /**
   * Data type, dependent on data length this can be an 8-bit or 16-bit uint.
   */
  using Data = std::conditional_t<(SS.data_size > 8), uint16_t, uint8_t>;

  /** Transmit DMA channel for this SPI peripheral. */
  using TxDmaChannel = DmaChannel<Id, SpiDmaRequest::Tx>;
  /** Receive DMA channel for this SPI peripheral. */
  using RxDmaChannel = DmaChannel<Id, SpiDmaRequest::Rx>;

  /**
   * @brief Interrupt handler. Forwards to the ST HAL SPI interrupt handler.
   */
  void HandleInterrupt() noexcept { HAL_SPI_IRQHandler(&hspi); }

  /**
   * @brief Returns the SPI baud rate.
   * @return SPI baud rate.
   */
  [[nodiscard]] static constexpr hstd::Frequency auto BaudRate() noexcept {
    constexpr auto f_src = SpiSourceFrequency().template As<hstd::Hz>();

    // If the prescaler value is 2^N, then the highest nibble of Presc is N-1.
    constexpr auto Presc         = FindPrescalerValue();
    constexpr auto PrescDivShift = static_cast<uint32_t>(Presc) >> 28U;
    constexpr auto PrescDiv      = 1U << (PrescDivShift + 1);

    // Edge case, bypass has value 8.
    if (PrescDivShift == 8) {
      return SpiSourceFrequency().template As<hstd::Hz>();
    }

    // Divide source clock by prescaler to obtain SPI baud rate.
    return (f_src / PrescDiv).template As<hstd::Hz>();
  }

  /**
   * @brief Singleton constructor.
   *
   * @return Singleton instance.
   */
  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  /**
   * @brief Transmits data over the SPI perihperal. Blocks the current RTOS
   * task until transmission is complete. Returns whether transmission was
   * successful or timed out.
   *
   * @tparam D SPI data type.
   * @param buffer Buffer that should be transmitted.
   * @param timeout Timeout to wait for transmission.
   * @return Whether transmission was successfully completed.
   */
  template <SpiData<SS> D>
  bool Transmit(std::span<const D> buffer, hstd::Duration auto timeout)
    requires(SS.operating_mode == SpiOperatingMode::DmaRtos)
  {
    Write(buffer, SpiImplBase<SS.operating_mode, Rest...>::event_group,
          SpiImplBase<SS.operating_mode, Rest...>::TxDoneBit);

    return SpiImplBase<SS.operating_mode, Rest...>::event_group
        .Wait(SpiImplBase<SS.operating_mode, Rest...>::TxDoneBit, timeout)
        .has_value();
  }

  /**
   * @brief Transmits data over the SPI peripheral. When transmission is done,
   * the bits in \c bitmask are set in the \c event_group RTOS event group.
   *
   * @tparam D SPI data type.
   * @param buffer Buffer that should be transmitted.
   * @param event_group Event group in which a bit should be set when data is
   * transmitted.
   * @param bitmask Bitmask that should be set in \c event_group when data is
   * transmitted.
   */
  template <SpiData<SS> D>
  bool
  Transmit(std::span<D>                                          buffer,
           typename SpiImplBase<SS.operating_mode, Rest...>::EG& event_group,
           uint32_t                                              bitmask)
    requires(SS.operating_mode == SpiOperatingMode::DmaRtos)
  {
    SpiImplBase<SS.operating_mode, Rest...>::tx_event_group =
        std::make_pair(&event_group, bitmask);

    return HAL_SPI_Transmit_DMA(
               &hspi, hstd::ReinterpretSpan<const uint8_t>(buffer).data(),
               buffer.size_bytes())
           == HAL_OK;
  }

 protected:
  /**
   * @brief Constructor for SPI running in \c Poll mode.
   *
   * @param pinout SPI peripheral pinout.
   */
  explicit SpiImpl(Pinout pinout) noexcept
    requires(SS.operating_mode == SpiOperatingMode::Poll)
  {
    PinoutHelper::SetupPins(pinout);
    EnableSpiClock<Id>();
    SetupSpiMaster();
  }

  SpiImpl(hal::Dma auto& dma, Pinout pinout) noexcept
    requires(SS.operating_mode == SpiOperatingMode::Dma
             || SS.operating_mode == SpiOperatingMode::DmaRtos)
  {
    // Validate DMA configuration
    using Dma = std::decay_t<decltype(dma)>;
    static_assert(hstd::Implies(hal::SpiTransmitEnabled(SS.transmission_type),
                                Dma::template ChannelEnabled<TxDmaChannel>()),
                  "If the transmission mode allows transmission, the TX DMA "
                  "channel must be enabled");
    static_assert(hstd::Implies(hal::SpiReceiveEnabled(SS.transmission_type),
                                Dma::template ChannelEnabled<RxDmaChannel>()),
                  "If the transmission mode allows transmission, the RX DMA "
                  "channel must be enabled");

    // Set up pins and SPI master
    PinoutHelper::SetupPins(pinout);
    if (!SetupSourceClock<Id, SS.source_clock>()) {
      while (true) {}
    }
    EnableSpiClock<Id>();
    if (!SetupSpiMaster()) {
      while (true) {}
    }

    // Setup Tx and Rx DMA channels
    if constexpr (hal::SpiTransmitEnabled(SS.transmission_type)) {
      auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
          hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
          hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmatx, htxdma);
      this->hdma_tx = &htxdma;
    }
    if constexpr (hal::SpiReceiveEnabled(SS.transmission_type)) {
      auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
          hal::DmaDirection::PeriphToMem, hal::DmaMode::Normal,
          hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmarx, hrxdma);
      this->hdma_rx = &hrxdma;
    }

    // Enable SPI interrupts.
    EnableSpiInterrupt<Id, Impl>();
  }

  /**
   * @brief Receive complete callback.
   */
  void ReceiveComplete() noexcept {
    // Handle DMA+RTOS callback through event group.
    if constexpr (SS.operating_mode == SpiOperatingMode::DmaRtos) {
      auto& [eg, bitmask] =
          SpiImplBase<SS.operating_mode, Rest...>::rx_event_group;
      eg->SetBitsFromInterrupt(bitmask);
    }

    // Handle DMA callback through SpiReceiveCallback.
    if constexpr (SS.operating_mode == SpiOperatingMode::Dma) {
      if constexpr (hal::AsyncRxSpiMaster<Impl>) {
        static_cast<SpiImpl*>(this)->SpiReceiveCallback();
      }
    }
  }

  /**
   * @brief Transmit complete calback.
   */
  void TransmitComplete() noexcept {
    // Handle DMA+RTOS callback through event group.
    if constexpr (SS.operating_mode == SpiOperatingMode::DmaRtos) {
      auto& [eg, bitmask] =
          SpiImplBase<SS.operating_mode, Rest...>::tx_event_group;
      eg->SetBitsFromInterrupt(bitmask);
    }

    // Handle DMA callback through SpiReceiveCallback.
    if constexpr (SS.operating_mode == SpiOperatingMode::Dma) {
      if constexpr (hal::AsyncTxSpiMaster<Impl>) {
        static_cast<SpiImpl*>(this)->SpiTransmitCallback();
      }
    }
  }

  SPI_HandleTypeDef hspi{};

 private:
  /**
   * @brief Returns the Source clock frequency to this SPI peripheral as
   * given by the SPI settings \c SS and clock settings <c>CS</c>.
   *
   * @return Source clock frequency to this SPI peripheral.
   */
  [[nodiscard]] static constexpr hstd::Frequency auto
  SpiSourceFrequency() noexcept {
    using enum SpiId;
    using enum SpiSourceClock;

    // SPI1/2/3
    if (Id == Spi1 || Id == Spi2 || Id == Spi3) {
      hstd::Assert(
          hstd::IsOneOf<Pll1Q, Pll2P, Pll3P, AudioClk, Per>(SS.source_clock),
          "SPI 1, 2 and 3 clock source must be valid");

      switch (SS.source_clock) {
      case Pll1Q: return CS.pll.pll1.OutputQ(CS.Pll1SourceClockFrequency());
      case Pll2P: return CS.pll.pll2.OutputP(CS.Pll2SourceClockFrequency());
      case Pll3P: return CS.pll.pll3.OutputP(CS.Pll3SourceClockFrequency());
      case AudioClk:
        hstd::Unimplemented(SS.source_clock == AudioClk, "AUDIOCLK SPI source");
      case Per: hstd::Unimplemented(SS.source_clock == Per, "PER SPI source");
      default: std::unreachable();
      }
    }

    // SPI4
    if (Id == Spi4) {
      hstd::Assert(
          hstd::IsOneOf<Pclk2, Pll2Q, Hsi, Csi, Hse, Per>(SS.source_clock),
          "SPI 4 clock source must be valid");

      switch (SS.source_clock) {
      case Pclk2:
        return CS.system_clock_settings.Apb2PeripheralsClockFrequency(
            CS.SysClkSourceClockFrequency());
      case Pll2Q: return CS.pll.pll2.OutputQ(CS.Pll2SourceClockFrequency());
      case Hsi: return CS.HsiFrequency();
      case Csi: return CS.CsiFrequency();
      case Hse: return CS.HseFrequency();
      case Per: hstd::Unimplemented(SS.source_clock == Per, "PER SPI source");
      default: std::unreachable();
      }
    }

    std::unreachable();
  }

  /**
   * @brief Finds the SPI baud rate prescaler that results in the baud rate that
   * is closest as possible to the baud rate requested in <c>SS</c>.
   *
   * @return Optimal SPI baud rate prescaler value.
   */
  [[nodiscard]] static constexpr SpiBaudPrescaler
  FindPrescalerValue() noexcept {
    const auto f_src = SpiSourceFrequency().template As<hstd::Hz>();

    const std::array<std::tuple<SpiBaudPrescaler, hstd::Hz>, 8> options{{
        {SpiBaudPrescaler::Prescale2, f_src / 2},
        {SpiBaudPrescaler::Prescale4, f_src / 4},
        {SpiBaudPrescaler::Prescale8, f_src / 8},
        {SpiBaudPrescaler::Prescale16, f_src / 16},
        {SpiBaudPrescaler::Prescale32, f_src / 32},
        {SpiBaudPrescaler::Prescale64, f_src / 64},
        {SpiBaudPrescaler::Prescale128, f_src / 128},
        {SpiBaudPrescaler::Prescale256, f_src / 256},
    }};

    auto best_err      = std::numeric_limits<uint32_t>::max();
    auto best_prescale = SpiBaudPrescaler::Prescale2;

    const auto desired = SS.frequency.template As<hstd::Hz>();

    for (const auto [prescale, actual_baud] : options) {
      const auto err = (actual_baud > desired ? (actual_baud - desired)
                                              : (desired - actual_baud))
                           .count;

      if (err < best_err) {
        best_err      = err;
        best_prescale = prescale;
      }
    }

    return best_prescale;
  }

  bool SetupSpiMaster() noexcept {
    constexpr auto Presc = FindPrescalerValue();

    hspi.Instance = GetSpiPointer(Id);
    hspi.Init     = {
            .Mode        = SPI_MODE_MASTER,
            .Direction   = ToHalMasterDirection(SS.transmission_type),
            .DataSize    = ToHalDataSize(SS.data_size),
            .CLKPolarity = SPI_POLARITY_LOW,
            .CLKPhase    = SPI_PHASE_1EDGE,
            .NSS         = SS.hardware_cs ? SPI_NSS_HARD_OUTPUT : SPI_NSS_SOFT,
            .BaudRatePrescaler = static_cast<uint32_t>(Presc),
            .FirstBit          = static_cast<uint32_t>(SS.bit_order),
            .TIMode            = SPI_TIMODE_DISABLE,
            .CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
            .CRCPolynomial     = 7,
            .CRCLength         = SPI_CRC_LENGTH_DATASIZE,
            .NSSPMode          = SPI_NSS_PULSE_DISABLE,
            .NSSPolarity       = SPI_NSS_POLARITY_LOW,
            .FifoThreshold     = SPI_FIFO_THRESHOLD_01DATA,
    };

    return HAL_SPI_Init(&hspi) == HAL_OK;
  }
};

export template <SpiId Id>
/**
 * Marker class for SPI peripherals
 * @tparam Id SPI id
 */
class Spi : public hal::UnusedPeripheral<Spi<Id>> {
 public:
  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);
  friend void ::HAL_SPI_TxCpltCallback(SPI_HandleTypeDef*);

  constexpr void HandleInterrupt() noexcept {}

 protected:
  void ReceiveComplete() noexcept {}
  void TransmitComplete() noexcept {}

  SPI_HandleTypeDef hspi{};
};

export using Spi1 = Spi<SpiId::Spi1>;
export using Spi2 = Spi<SpiId::Spi2>;
export using Spi3 = Spi<SpiId::Spi3>;
export using Spi4 = Spi<SpiId::Spi4>;

}   // namespace stm32h5
