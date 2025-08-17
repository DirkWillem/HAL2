module;

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

export template <SpiId Id, hal::SpiMode M, hal::SpiTransmissionType TT>
struct SpiPinoutHelper;

export template <SpiId Id>
struct SpiPinoutHelper<Id, hal::SpiMode::Master,
                       hal::SpiTransmissionType::FullDuplex> {
  struct Pinout {
    consteval Pinout(PinId mosi, PinId miso, PinId sck,
                     hal::PinPull pull_mosi = hal::PinPull::NoPull,
                     hal::PinPull pull_miso = hal::PinPull::NoPull,
                     hal::PinPull pull_sck  = hal::PinPull::NoPull) noexcept
        : mosi{mosi}
        , miso{miso}
        , sck{sck}
        , pull_mosi{pull_mosi}
        , pull_miso{pull_miso}
        , pull_sck{pull_sck} {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
      assert(("MOSI pin must be valid",
              hal::FindPinAFMapping(SpiMosiPinMappings, Id, mosi).has_value()));
      assert(("MISO pin must be valid",
              hal::FindPinAFMapping(SpiMisoPinMappings, Id, miso).has_value()));
      assert(("SCK pin must be valid",
              hal::FindPinAFMapping(SpiSckPinMappings, Id, sck).has_value()));
#pragma GCC diagnostic pop
    }

    PinId mosi;
    PinId miso;
    PinId sck;

    hal::PinPull pull_mosi;
    hal::PinPull pull_miso;
    hal::PinPull pull_sck;
  };

  static void SetupPins(const Pinout& p) {
    Pin::InitializeAlternate(
        p.mosi, hal::FindPinAFMapping(SpiMosiPinMappings, Id, p.mosi)->af,
        p.pull_mosi);
    Pin::InitializeAlternate(
        p.miso, hal::FindPinAFMapping(SpiMisoPinMappings, Id, p.miso)->af,
        p.pull_miso);
    Pin::InitializeAlternate(
        p.sck, hal::FindPinAFMapping(SpiSckPinMappings, Id, p.sck)->af,
        p.pull_sck);
  }
};

export template <SpiId Id>
struct SpiPinoutHelper<Id, hal::SpiMode::Master,
                       hal::SpiTransmissionType::TxOnly> {
  struct Pinout {
    consteval Pinout(PinId mosi, PinId sck,
                     hal::PinPull pull_mosi = hal::PinPull::NoPull,
                     hal::PinPull pull_sck  = hal::PinPull::NoPull) noexcept
        : mosi{mosi}
        , sck{sck}
        , pull_mosi{pull_mosi}
        , pull_sck{pull_sck} {
      assert(("MOSI pin must be valid",
              hal::FindPinAFMapping(SpiMosiPinMappings, Id, mosi).has_value()));
      assert(("SCK pin must be valid",
              hal::FindPinAFMapping(SpiSckPinMappings, Id, sck).has_value()));
    }

    PinId mosi;
    PinId sck;

    hal::PinPull pull_mosi;
    hal::PinPull pull_sck;
  };

  static void SetupPins(const Pinout& p) {
    Pin::InitializeAlternate(
        p.mosi, hal::FindPinAFMapping(SpiMosiPinMappings, Id, p.mosi)->af,
        p.pull_mosi);
    Pin::InitializeAlternate(
        p.sck, hal::FindPinAFMapping(SpiSckPinMappings, Id, p.sck)->af,
        p.pull_sck);
  }
};

export template <SpiId Id>
struct SpiPinoutHelper<Id, hal::SpiMode::Master,
                       hal::SpiTransmissionType::RxOnly> {
  struct Pinout {
    consteval Pinout(PinId miso, PinId sck,
                     hal::PinPull pull_miso = hal::PinPull::NoPull,
                     hal::PinPull pull_sck  = hal::PinPull::NoPull) noexcept
        : miso{miso}
        , sck{sck}
        , pull_miso{pull_miso}
        , pull_sck{pull_sck} {
      assert(("MISO pin must be valid",
              hal::FindPinAFMapping(SpiMisoPinMappings, Id, miso).has_value()));
      assert(("SCK pin must be valid",
              hal::FindPinAFMapping(SpiSckPinMappings, Id, sck).has_value()));
    }

    PinId miso;
    PinId sck;

    hal::PinPull pull_miso;
    hal::PinPull pull_sck;
  };

  static void SetupPins(const Pinout& p) {
    Pin::InitializeAlternate(
        p.miso, hal::FindPinAFMapping(SpiMisoPinMappings, Id, p.miso)->af,
        p.pull_miso);
    Pin::InitializeAlternate(
        p.sck, hal::FindPinAFMapping(SpiMisoPinMappings, Id, p.sck)->af,
        p.pull_sck);
  }
};

export template <SpiId Id>
struct SpiPinoutHelper<Id, hal::SpiMode::Master,
                       hal::SpiTransmissionType::HalfDuplex>
    : public SpiPinoutHelper<Id, hal::SpiMode::Master,
                             hal::SpiTransmissionType::TxOnly> {};

void EnableSpiClk(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: __HAL_RCC_SPI1_CLK_ENABLE(); break;
  case SpiId::Spi2: __HAL_RCC_SPI2_CLK_ENABLE(); break;
#ifdef HAS_SPI3
  case SpiId::Spi3: __HAL_RCC_SPI3_CLK_ENABLE(); break;
#endif
  }
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

[[nodiscard]] static constexpr uint32_t
ToHalMasterDirection(const hal::SpiTransmissionType tt) noexcept {
  switch (tt) {
  case hal::SpiTransmissionType::FullDuplex: return SPI_DIRECTION_2LINES;
  case hal::SpiTransmissionType::HalfDuplex: return SPI_DIRECTION_1LINE;
  case hal::SpiTransmissionType::TxOnly: return SPI_DIRECTION_2LINES;
  case hal::SpiTransmissionType::RxOnly: return SPI_DIRECTION_2LINES_RXONLY;
  }

  std::unreachable();
}

[[nodiscard]] static constexpr uint32_t ToHalDataSize(unsigned size) noexcept {
  // ReSharper disable once CppRedundantParentheses
  return (size - 1) << 8U;
}

void SetupSpiMaster(SpiId id, SPI_HandleTypeDef& hspi,
                    SpiBaudPrescaler baud_prescaler, unsigned data_size,
                    hal::SpiTransmissionType transmission_type) noexcept {
  EnableSpiClk(id);

  hspi.Instance = GetSpiPointer(id);
  hspi.Init     = {
          .Mode              = SPI_MODE_MASTER,
          .Direction         = ToHalMasterDirection(transmission_type),
          .DataSize          = ToHalDataSize(data_size),
          .CLKPolarity       = SPI_POLARITY_LOW,
          .CLKPhase          = SPI_PHASE_1EDGE,
          .NSS               = SPI_NSS_SOFT,
          .BaudRatePrescaler = static_cast<uint32_t>(baud_prescaler),
          .FirstBit          = SPI_FIRSTBIT_MSB,
          .TIMode            = SPI_TIMODE_DISABLE,
          .CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
          .CRCPolynomial     = 7,
          .CRCLength         = SPI_CRC_LENGTH_DATASIZE,
          .NSSPMode          = SPI_NSS_PULSE_DISABLE,
  };

  HAL_SPI_Init(&hspi);
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

export template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<Id, SpiDmaRequest::Tx, Prio>;

export template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<Id, SpiDmaRequest::Rx, Prio>;

export enum class SpiOperatingMode { Poll, Dma };

export template <typename Impl, SpiId Id, ClockSettings CS, SpiOperatingMode OM,
                 unsigned DS, hal::SpiMode M, hal::SpiTransmissionType TT>
  requires(DS == 8 || DS == 16)
class SpiImpl : public hal::UsedPeripheral {
  using PinoutHelper = SpiPinoutHelper<Id, M, TT>;

  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);

 public:
  static constexpr auto Mode             = M;
  static constexpr auto TransmissionType = TT;
  static constexpr auto DataSize         = DS;
  using Pinout                           = PinoutHelper::Pinout;
  using Data         = std::conditional_t<(DS > 8), uint16_t, uint8_t>;
  using TxDmaChannel = DmaChannel<Id, SpiDmaRequest::Tx>;
  using RxDmaChannel = DmaChannel<Id, SpiDmaRequest::Rx>;
  using Type         = Impl;

  void HandleInterrupt() noexcept { HAL_SPI_IRQHandler(&hspi); }

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  [[nodiscard]] bool ReceiveBlocking(std::span<Data> into,
                                     uint32_t        timeout) noexcept
    requires(hal::SpiReceiveEnabled(TT))
  {
    return HAL_SPI_Receive(&hspi, reinterpret_cast<uint8_t*>(into.data()),
                           into.size(), timeout)
           == HAL_OK;
  }

  [[nodiscard]] bool Receive(std::span<Data> into) noexcept
    requires(OM == SpiOperatingMode::Dma && hal::SpiReceiveEnabled(TT))
  {
    return HAL_SPI_Receive_DMA(&hspi, reinterpret_cast<uint8_t*>(into.data()),
                               into.size())
           == HAL_OK;
  }

  [[nodiscard]] bool TransmitBlocking(std::span<Data> data,
                                      uint32_t        timeout) noexcept
    requires(hal::SpiTransmitEnabled(TT))
  {
    return HAL_SPI_Transmit(&hspi, reinterpret_cast<uint8_t*>(data.data()),
                            data.size(), timeout)
           == HAL_OK;
  }

  [[nodiscard]] bool Transmit(std::span<Data> data) noexcept
    requires(OM == SpiOperatingMode::Dma && hal::SpiTransmitEnabled(TT))
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

  SpiImpl(Pinout pinout, SpiBaudPrescaler baud_prescaler)
    requires(OM == SpiOperatingMode::Poll)
  {
    PinoutHelper::SetupPins(pinout);

    SetupSpiMaster(Id, hspi, baud_prescaler, DS, TT);
  }

  SpiImpl(hal::Dma auto& dma, Pinout pinout,
          hstd::Frequency auto clock_frequency)
    requires(OM == SpiOperatingMode::Dma)
  {
    // Validate DMA instance
    using Dma = std::decay_t<decltype(dma)>;
    static_assert(hstd::Implies(hal::SpiTransmitEnabled(TT),
                                Dma::template ChannelEnabled<TxDmaChannel>()),
                  "If the transmission mode allows transmission, the TX DMA "
                  "channel must be enabled");
    static_assert(hstd::Implies(hal::SpiReceiveEnabled(TT),
                                Dma::template ChannelEnabled<RxDmaChannel>()),
                  "If the transmission mode allows transmission, the RX DMA "
                  "channel must be enabled");

    // Set up pins
    PinoutHelper::SetupPins(pinout);

    // Initialize SPI master
    const auto presc = FindPrescalerValue<CS>(clock_frequency);
    SetupSpiMaster(Id, hspi, presc, DS, TT);

    // Set up DMA channels
    if constexpr (hal::SpiTransmitEnabled(TT)) {
      auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
          hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
          hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmatx, htxdma);
    }

    if constexpr (hal::SpiReceiveEnabled(TT)) {
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