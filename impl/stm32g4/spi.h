#pragma once

#include <limits>
#include <span>
#include <tuple>

#include <hal/spi.h>

#include <stm32g4/clocks.h>
#include <stm32g4/dma.h>
#include <stm32g4/peripheral_ids.h>
#include <stm32g4/pin.h>

#include <stm32g4/mappings/spi_i2s_pin_mapping.h>

namespace stm32g4 {

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

template <auto CF>
  requires ClockFrequencies<decltype(CF)>
[[nodiscard]] consteval uint32_t
FindPrescalerValue(SpiId id, ct::Frequency auto baud_rate) {
  std::array<std::tuple<SpiBaudPrescaler, ct::Hz>, 8> options{{
      {SpiBaudPrescaler::Prescale2,
       (CF.PeripheralClkFreq(id) / 2).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale4,
       (CF.PeripheralClkFreq(id) / 4).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale8,
       (CF.PeripheralClkFreq(id) / 8).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale16,
       (CF.PeripheralClkFreq(id) / 16).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale32,
       (CF.PeripheralClkFreq(id) / 32).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale64,
       (CF.PeripheralClkFreq(id) / 64).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale128,
       (CF.PeripheralClkFreq(id) / 128).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale256,
       (CF.PeripheralClkFreq(id) / 256).template As<ct::Hz>()},
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

  return static_cast<uint32_t>(best_prescale);
}

}   // namespace detail

template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<Id, SpiDmaRequest::Tx, Prio>;

template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<Id, SpiDmaRequest::Rx, Prio>;

enum class SpiOperatingMode { Poll, Dma };

template <typename Impl, SpiId Id, auto CF, SpiOperatingMode OM, unsigned DS,
          hal::SpiMode M, hal::SpiTransmissionType TT>
  requires ClockFrequencies<decltype(CF)> && (DS >= 4 && DS <= 16)
class SpiImpl : public hal::UsedPeripheral {
  using PinoutHelper = detail::SpiPinoutHelper<Id, M, TT>;

 public:
  static constexpr auto Mode             = M;
  static constexpr auto TransmissionType = TT;
  static constexpr auto DataSize         = DS;
  using Pinout                           = typename PinoutHelper::Pinout;
  using Data = std::conditional_t<(DS > 8), uint16_t, uint8_t>;

  void HandleInterrupt() noexcept { HAL_SPI_IRQHandler(&hspi); }

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  [[nodiscard]] bool ReceiveBlocking(std::span<Data> into,
                                     uint32_t        timeout) noexcept
    requires(TT == hal::SpiTransmissionType::FullDuplex
             || TT == hal::SpiTransmissionType::HalfDuplex
             || TT == hal::SpiTransmissionType::RxOnly)
  {
    return HAL_SPI_Receive(&hspi, reinterpret_cast<uint8_t*>(into.data()),
                           into.size(), timeout)
           == HAL_OK;
  }

 protected:
  SpiImpl(Pinout pinout, SpiBaudPrescaler baud_prescaler) {
//    constexpr auto presc = detail::FindPrescalerValue<CF>(Id, ct::Hz{100'000});

    PinoutHelper::SetupPins(pinout);

    detail::SetupSpiMaster(Id, hspi, SpiBaudPrescaler::Prescale2, DS, TT);
  }

  SpiImpl(hal::Dma auto& dma, Pinout pinout, SpiBaudPrescaler baud_prescaler)
    requires(OM == SpiOperatingMode::Dma)
  {}

 private:
  SPI_HandleTypeDef hspi{};
};

template <SpiId Id>
/**
 * Marker class for UART peripherals
 * @tparam Id UART id
 */
class Spi : public hal::UnusedPeripheral<Spi<Id>> {
 public:
  constexpr void HandleInterrupt() noexcept {}

 protected:
  SPI_HandleTypeDef huart{};
};

using Spi1 = Spi<SpiId::Spi1>;
using Spi2 = Spi<SpiId::Spi2>;
using Spi3 = Spi<SpiId::Spi3>;
using Spi4 = Spi<SpiId::Spi4>;

}   // namespace stm32g4