#pragma once

#include <span>

#include <constexpr_tools/logic.h>

#include <hal/spi.h>

#include <stm32g0/clocks.h>
#include <stm32g0/dma.h>

#include <stm32g0/mappings/spi_i2s_pin_mapping.h>

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

namespace detail {

template <SpiId Id, hal::SpiMode M, hal::SpiTransmissionType TT>
struct SpiPinoutHelper;

template <SpiId Id>
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
      assert(("MOSI pin must be valid",
              hal::FindPinAFMapping(SpiMosiPinMappings, Id, mosi).has_value()));
      assert(("MISO pin must be valid",
              hal::FindPinAFMapping(SpiMisoPinMappings, Id, miso).has_value()));
      assert(("SCK pin must be valid",
              hal::FindPinAFMapping(SpiSckPinMappings, Id, sck).has_value()));
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

template <SpiId Id>
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

template <SpiId Id>
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

template <SpiId Id>
struct SpiPinoutHelper<Id, hal::SpiMode::Master,
                       hal::SpiTransmissionType::HalfDuplex>
    : public SpiPinoutHelper<Id, hal::SpiMode::Master,
                             hal::SpiTransmissionType::TxOnly> {};

void EnableSpiClk(SpiId id) noexcept;
void EnableSpiInterrupt(SpiId id) noexcept;
void SetupSpiMaster(SpiId id, SPI_HandleTypeDef& hspi,
                    SpiBaudPrescaler baud_prescaler, unsigned data_size,
                    hal::SpiTransmissionType transmission_type) noexcept;

template <ClockSettings CS>
[[nodiscard]] constexpr SpiBaudPrescaler
FindPrescalerValue(ct::Frequency auto baud_rate) {
  const auto f_src =
      CS.system_clock_settings
          .ApbPeripheralsClockFrequency(CS.SysClkSourceClockFrequency())
          .As<ct::Hz>();

  std::array<std::tuple<SpiBaudPrescaler, ct::Hz>, 8> options{{
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

  const auto desired = baud_rate.template As<ct::Hz>();

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

}   // namespace detail

template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<Id, SpiDmaRequest::Tx, Prio>;

template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<Id, SpiDmaRequest::Rx, Prio>;

enum class SpiOperatingMode { Poll, Dma };

template <typename Impl, SpiId Id, ClockSettings CS, SpiOperatingMode OM,
          unsigned DS, hal::SpiMode M, hal::SpiTransmissionType TT>
  requires(DS == 8 || DS == 16)
class SpiImpl : public hal::UsedPeripheral {
  using PinoutHelper = detail::SpiPinoutHelper<Id, M, TT>;

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

    detail::SetupSpiMaster(Id, hspi, baud_prescaler, DS, TT);
  }

  SpiImpl(hal::Dma auto& dma, Pinout pinout, ct::Frequency auto clock_frequency)
    requires(OM == SpiOperatingMode::Dma)
  {
    // Validate DMA instance
    static_assert(ct::Implies(hal::SpiTransmitEnabled(TT),
                              dma.template ChannelEnabled<TxDmaChannel>()),
                  "If the transmission mode allows transmission, the TX DMA "
                  "channel must be enabled");
    static_assert(ct::Implies(hal::SpiReceiveEnabled(TT),
                              dma.template ChannelEnabled<RxDmaChannel>()),
                  "If the transmission mode allows transmission, the RX DMA "
                  "channel must be enabled");

    // Set up pins
    PinoutHelper::SetupPins(pinout);

    // Initialize SPI master
    const auto presc = detail::FindPrescalerValue<CS>(clock_frequency);
    detail::SetupSpiMaster(Id, hspi, presc, DS, TT);

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
          hal::DmaDataWidth::Byte, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmarx, hrxdma);
    }
  }

 private:
  std::span<std::byte> rx_buf{};
  SPI_HandleTypeDef    hspi{};
};

template <SpiId Id>
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

using Spi1 = Spi<SpiId::Spi1>;
using Spi2 = Spi<SpiId::Spi2>;
using Spi3 = Spi<SpiId::Spi3>;

}   // namespace stm32g0