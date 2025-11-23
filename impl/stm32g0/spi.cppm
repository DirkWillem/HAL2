module;

#include <chrono>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <stm32g0xx_hal.h>
#include <stm32g0xx_hal_spi.h>

#include "internal/peripheral_availability.h"

export module hal.stm32g0:spi;

import hstd;
import hal.abstract;

import rtos.concepts;

import :clocks;
import :dma;
import :peripherals;
import :pin_mapping.spi_i2s;

extern "C" {

[[maybe_unused]] void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);
}

namespace stm32g0 {

export enum class SpiOperatingMode { Poll, Dma };

export template <typename F = hstd::Hz>
struct SpiSettings {
  F                        frequency;             //!< SPI Frequency
  SpiOperatingMode         operating_mode;        //!< SPI operating mode
  hal::SpiMode             mode;                  //!< SPI mode
  hal::SpiTransmissionType transmission_type;     //!< SPI transmission type
  unsigned                 data_size;             //!< SPI data size
  bool                     hardware_cs = false;   //!< Hardware Chip Select
};

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

template <typename F>
consteval bool CheckSpiConfig(SpiSettings<F> settings, hal::SpiMode mode,
                              hal::SpiTransmissionType tt, bool hw_cs) {
  return settings.mode == mode && settings.transmission_type == tt
         && settings.hardware_cs == hw_cs;
}

/**
 * Struct that contains the pin configuration of a SPI instance. Exact pins
 * that are expected is dependent on the SPI configuration.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
struct SpiPinout;

export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::FullDuplex, false))
struct SpiPinout<S> {
  PinId mosi;   //!< MOSI pin
  PinId miso;   //!< MISO pin
  PinId sck;    //!< Clock pin

  hal::PinPull pull_mosi = hal::PinPull::NoPull;   //!< MOSI pin pull
  hal::PinPull pull_miso = hal::PinPull::NoPull;   //!< MISO pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
};

export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::FullDuplex, true))
struct SpiPinout<S> {
  PinId mosi;   //!< MOSI pin
  PinId miso;   //!< MISO pin
  PinId sck;    //!< Clock pin
  PinId cs;     //!< Chip Select pin

  hal::PinPull pull_mosi = hal::PinPull::NoPull;   //!< MOSI pin pull
  hal::PinPull pull_miso = hal::PinPull::NoPull;   //!< MISO pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
  hal::PinPull pull_cs   = hal::PinPull::NoPull;   //!< Chip Select pin pull
};

template <typename PO>
concept MosiPin = requires(PO p) {
  { p.mosi } -> std::convertible_to<PinId>;
  { p.pull_mosi } -> std::convertible_to<hal::PinPull>;
};

template <typename PO>
concept MisoPin = requires(PO p) {
  { p.miso } -> std::convertible_to<PinId>;
  { p.pull_miso } -> std::convertible_to<hal::PinPull>;
};

template <typename PO>
concept SckPin = requires(PO p) {
  { p.sck } -> std::convertible_to<PinId>;
  { p.pull_sck } -> std::convertible_to<hal::PinPull>;
};

template <typename PO>
concept CsPin = requires(PO p) {
  { p.cs } -> std::convertible_to<PinId>;
  { p.pull_cs } -> std::convertible_to<hal::PinPull>;
};

export template <SpiId Id, SpiSettings S>
struct SpiPinoutHelper {
  struct Pinout {
    consteval Pinout(SpiPinout<S> pinout)
        : pinout{pinout} {
      // Validate pinout
      if constexpr (MosiPin<SpiPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(SpiMosiPinMappings, Id, pinout.mosi)
                         .has_value(),
                     "MOSI pin must be valid");
      }
      if constexpr (MisoPin<SpiPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(SpiMisoPinMappings, Id, pinout.miso)
                         .has_value(),
                     "MISO pin must be valid");
      }
      if constexpr (SckPin<SpiPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(SpiSckPinMappings, Id, pinout.sck)
                         .has_value(),
                     "SCK pin must be valid");
      }
      if constexpr (CsPin<SpiPinout<S>>) {
        hstd::Assert(
            hal::FindPinAFMapping(SpiNssPinMappings, Id, pinout.cs).has_value(),
            "CS pin must be valid");
      }
    }

    SpiPinout<S> pinout;
  };

  static void SetupPins(const Pinout& p) {
    if constexpr (MosiPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.mosi,
          hal::FindPinAFMapping(SpiMosiPinMappings, Id, p.pinout.mosi)->af,
          p.pinout.pull_mosi);
    }
    if constexpr (MisoPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.miso,
          hal::FindPinAFMapping(SpiMisoPinMappings, Id, p.pinout.miso)->af,
          p.pinout.pull_miso);
    }
    if constexpr (SckPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.sck,
          hal::FindPinAFMapping(SpiSckPinMappings, Id, p.pinout.sck)->af,
          p.pinout.pull_sck);
    }
    if constexpr (CsPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.cs,
          hal::FindPinAFMapping(SpiNssPinMappings, Id, p.pinout.cs)->af,
          p.pinout.pull_cs);
    }
  }
};

template <SpiId Id>
void EnableSpiClk() noexcept {
  using enum SpiId;

  if constexpr (Id == Spi1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    return;
  }
  if constexpr (Id == Spi2) {
    __HAL_RCC_SPI2_CLK_ENABLE();
    return;
  }
#ifdef HAS_SPI3
  if constexpr (Id == Spi3) {
    __HAL_RCC_SPI3_CLK_ENABLE();
    return;
  }
#endif
}

void EnableSpiInterrupt(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1:
    HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    break;
#if !defined(HAS_SPI3)
  case SpiId::Spi2:
    HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
    break;
#else
  case SpiId::Spi2: [[fallthrough]];
  case SpiId::Spi3:
    HAL_NVIC_SetPriority(SPI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI2_3_IRQn);
    break;
#endif
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
  return (size - 1) << 8U;
}

template <ClockSettings CS>
[[nodiscard]] constexpr SpiBaudPrescaler
FindPrescalerValue(hstd::Frequency auto baud_rate) {
  const auto f_src =
      CS.system_clock_settings
          .ApbPeripheralsClockFrequency(CS.SysClkSourceClockFrequency())
          .As<hstd::Hz>();

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

  const auto desired = baud_rate.template As<hstd::Hz>();

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

template <SpiId Id, ClockSettings CS, SpiSettings SS>
void SetupSpiMaster(SPI_HandleTypeDef& hspi) noexcept {
  EnableSpiClk<Id>();

  constexpr auto Presc = FindPrescalerValue<CS>(SS.frequency);

  hspi.Instance = GetSpiPointer<Id>();
  hspi.Init     = {
          .Mode              = SPI_MODE_MASTER,
          .Direction         = ToHalMasterDirection(SS.transmission_type),
          .DataSize          = ToHalDataSize(SS.data_size),
          .CLKPolarity       = SPI_POLARITY_LOW,
          .CLKPhase          = SPI_PHASE_1EDGE,
          .NSS               = SS.hardware_cs ? SPI_NSS_HARD_OUTPUT : SPI_NSS_SOFT,
          .BaudRatePrescaler = static_cast<uint32_t>(Presc),
          .FirstBit          = SPI_FIRSTBIT_MSB,
          .TIMode            = SPI_TIMODE_DISABLE,
          .CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
          .CRCPolynomial     = 7,
          .CRCLength         = SPI_CRC_LENGTH_DATASIZE,
          .NSSPMode          = SPI_NSS_PULSE_DISABLE,   // SPI_NSS_PULSE_ENABLE,
  };

  HAL_SPI_Init(&hspi);
}

export template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<Id, SpiDmaRequest::Tx, Prio>;

export template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<Id, SpiDmaRequest::Rx, Prio>;



export template <typename Impl, SpiId Id, ClockSettings CS, SpiSettings SS>
  requires(SS.data_size == 8 || SS.data_size == 16)
class SpiImpl : public hal::UsedPeripheral {
  using PinoutHelper = SpiPinoutHelper<Id, SS>;

  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);

 public:
  static constexpr auto Mode             = SS.mode;
  static constexpr auto TransmissionType = SS.transmission_type;
  static constexpr auto DataSize         = SS.data_size;
  using Pinout                           = PinoutHelper::Pinout;
  using Data = std::conditional_t<(SS.data_size > 8), uint16_t, uint8_t>;
  using TxDmaChannel = DmaChannel<Id, SpiDmaRequest::Tx>;
  using RxDmaChannel = DmaChannel<Id, SpiDmaRequest::Rx>;
  using Type         = Impl;

  void HandleInterrupt() noexcept { HAL_SPI_IRQHandler(&hspi); }

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  [[nodiscard]] bool ReceiveBlocking(std::span<Data>     into,
                                     hstd::Duration auto timeout) noexcept
    requires(hal::SpiReceiveEnabled(SS.transmission_type))
  {
    return HAL_SPI_Receive(
               &hspi, reinterpret_cast<uint8_t*>(into.data()), into.size(),
               std::chrono::duration_cast<std::chrono::milliseconds>(timeout)
                   .count())
           == HAL_OK;
  }

  [[nodiscard]] bool Receive(std::span<Data> into) noexcept
    requires(SS.operating_mode == SpiOperatingMode::Dma
             && hal::SpiReceiveEnabled(SS.transmission_type))
  {
    return HAL_SPI_Receive_DMA(&hspi, reinterpret_cast<uint8_t*>(into.data()),
                               into.size())
           == HAL_OK;
  }

  [[nodiscard]] bool TransmitBlocking(std::span<Data>     data,
                                      hstd::Duration auto timeout) noexcept
    requires(hal::SpiTransmitEnabled(SS.transmission_type))
  {
    return HAL_SPI_Transmit(
               &hspi, reinterpret_cast<uint8_t*>(data.data()), data.size(),
               std::chrono::duration_cast<std::chrono::milliseconds>(timeout)
                   .count())
           == HAL_OK;
  }
  [[nodiscard]] bool TransmitReceiveBlocking(std::span<Data> rx_data,
                                             std::span<Data> tx_data,
                                             uint32_t        timeout) noexcept
    requires(hal::SpiTransmitEnabled(SS.transmission_type))
  {
    const volatile auto result = HAL_SPI_TransmitReceive(
        &hspi, reinterpret_cast<uint8_t*>(tx_data.data()),
        reinterpret_cast<uint8_t*>(rx_data.data()),
        std::min(rx_data.size(), tx_data.size()), timeout);
    return result == HAL_OK;
  }

  [[nodiscard]] bool Transmit(std::span<Data> data) noexcept
    requires(SS.operating_mode == SpiOperatingMode::Dma
             && hal::SpiTransmitEnabled(SS.transmission_type))
  {
    return HAL_SPI_Transmit_DMA(&hspi, reinterpret_cast<uint8_t*>(data.data()),
                                data.size())
           == HAL_OK;
  }

 protected:
  void ReceiveComplete() noexcept {
    if constexpr (hal::AsyncRxSpiMaster<Impl>) {
      static_cast<Impl*>(this)->SpiReceiveCallback(rx_buf);
    }
  }

  SpiImpl(Pinout pinout)
    requires(SS.operating_mode == SpiOperatingMode::Poll)
  {
    PinoutHelper::SetupPins(pinout);
    SetupSpiMaster<Id, CS, SS>(hspi);
  }

  SpiImpl(hal::Dma auto& dma, Pinout pinout,
          hstd::Frequency auto clock_frequency)
    requires(SS.operating_mode == SpiOperatingMode::Dma)
  {
    // Validate DMA instance
    using Dma = std::decay_t<decltype(dma)>;
    static_assert(hstd::Implies(hal::SpiTransmitEnabled(SS.transmission_type),
                                Dma::template ChannelEnabled<TxDmaChannel>()),
                  "If the transmission mode allows transmission, the TX DMA "
                  "channel must be enabled");
    static_assert(hstd::Implies(hal::SpiReceiveEnabled(SS.transmission_type),
                                Dma::template ChannelEnabled<RxDmaChannel>()),
                  "If the transmission mode allows transmission, the RX DMA "
                  "channel must be enabled");

    // Set up pins
    PinoutHelper::SetupPins(pinout);

    // Initialize SPI master
    const auto presc = FindPrescalerValue<CS>(clock_frequency);
    SetupSpiMaster<Id, CS, SS>(hspi);

    // Set up DMA channels
    if constexpr (hal::SpiTransmitEnabled(SS.transmission_type)) {
      auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
          hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
          hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmatx, htxdma);
    }

    if constexpr (hal::SpiReceiveEnabled(SS.transmission_type)) {
      auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
          hal::DmaDirection::PeriphToMem, hal::DmaMode::Normal,
          hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmarx, hrxdma);
    }
  }

 private:
  std::span<std::byte> rx_buf{};
  SPI_HandleTypeDef    hspi{};
};

export template <SpiId Id>
/**
 * Marker class for UART peripherals
 * @tparam Id UART id
 */
class Spi : public hal::UnusedPeripheral<Spi<Id>> {
 public:
  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);

  constexpr void HandleInterrupt() noexcept {}

 protected:
  void ReceiveComplete() noexcept {}

  SPI_HandleTypeDef hspi{};
};

export using Spi1 = Spi<SpiId::Spi1>;
export using Spi2 = Spi<SpiId::Spi2>;
#ifdef HAS_SPI3
export using Spi3 = Spi<SpiId::Spi3>;
#endif

}   // namespace stm32g0