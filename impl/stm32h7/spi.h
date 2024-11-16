#pragma once

#include <limits>
#include <span>
#include <tuple>

#include <hal/spi.h>

#include <stm32h7/mappings/spi_i2s_pin_mapping.h>

#include "clocks.h"
#include "core.h"
#include "dma.h"

namespace stm32h7 {

namespace detail {

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
        p.sck, hal::FindPinAFMapping(SpiSckPinMappings, Id, p.sck)->af,
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

template <SpiId Id, ClockSettings CS>
[[nodiscard]] constexpr SpiBaudPrescaler
FindPrescalerValue(ct::Frequency auto baud_rate) {
  std::array<std::tuple<SpiBaudPrescaler, ct::Hz>, 8> options{{
      {SpiBaudPrescaler::Prescale2,
       (CS.PeripheralSourceClockFrequency(Id) / 2).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale4,
       (CS.PeripheralSourceClockFrequency(Id) / 4).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale8,
       (CS.PeripheralSourceClockFrequency(Id) / 8).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale16,
       (CS.PeripheralSourceClockFrequency(Id) / 16).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale32,
       (CS.PeripheralSourceClockFrequency(Id) / 32).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale64,
       (CS.PeripheralSourceClockFrequency(Id) / 64).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale128,
       (CS.PeripheralSourceClockFrequency(Id) / 128).template As<ct::Hz>()},
      {SpiBaudPrescaler::Prescale256,
       (CS.PeripheralSourceClockFrequency(Id) / 256).template As<ct::Hz>()},
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

template <Core C, SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<C, Id, SpiDmaRequest::Tx, Prio>;

template <Core C, SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<C, Id, SpiDmaRequest::Rx, Prio>;

enum class SpiOperatingMode { Poll, Dma };

template <typename Impl, SpiId Id, ClockSettings CS, SpiOperatingMode OM,
          unsigned DS, hal::SpiMode M, hal::SpiTransmissionType TT,
          typename DTO = void>
  requires(DS >= 4 && DS <= 16)
class SpiImpl : public hal::UsedPeripheral {
  static constexpr auto HasDataTypeOverride = !std::is_same_v<DTO, void>;

  static_assert(ct::Implies(std::is_same_v<DTO, std::byte>, DS == 8),
                "std::byte is only a valid data type if data size is 8");

  static_assert(
      ct::Implies(HasDataTypeOverride,
                  std::is_same_v<DTO, std::byte> || std::is_unsigned_v<DTO>),
      "Only unsigned integers and std::byte are valid data type overrides");

  friend void ::HAL_SPI_TxCpltCallback(SPI_HandleTypeDef*);
  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);
  using PinoutHelper = detail::SpiPinoutHelper<Id, M, TT>;

 public:
  static constexpr auto Mode             = M;
  static constexpr auto TransmissionType = TT;
  static constexpr auto DataSize         = DS;
  using TxDmaChannel = DmaChannel<CurrentCore, Id, SpiDmaRequest::Tx>;
  using RxDmaChannel = DmaChannel<CurrentCore, Id, SpiDmaRequest::Rx>;
  using Pinout       = typename PinoutHelper::Pinout;

  using DataInfer = std::conditional_t<(DS > 8), uint16_t, uint8_t>;
  using Data      = std::conditional_t<HasDataTypeOverride, DTO, DataInfer>;

  void HandleInterrupt() noexcept { HAL_SPI_IRQHandler(&hspi); }

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  [[nodiscard]] bool ReceiveBlocking(std::span<Data>   into,
                                     ct::Duration auto timeout) noexcept
    requires(hal::SpiReceiveEnabled(TT))
  {
    return HAL_SPI_Receive(
               &hspi, reinterpret_cast<uint8_t*>(into.data()), into.size(),
               std::chrono::duration_cast<std::chrono::milliseconds>(timeout)
                   .count())
           == HAL_OK;
  }

  [[nodiscard]] bool Receive(std::span<Data> into) noexcept
    requires(OM == SpiOperatingMode::Dma && hal::SpiReceiveEnabled(TT))
  {
    rx_buf = into;
    return HAL_SPI_Receive_DMA(&hspi, reinterpret_cast<uint8_t*>(rx_buf.data()),
                               rx_buf.size())
           == HAL_OK;
  }

  [[nodiscard]] bool Transmit(std::span<const Data> data) noexcept
    requires(OM == SpiOperatingMode::Dma && hal::SpiTransmitEnabled(TT))
  {
    return HAL_SPI_Transmit_DMA(&hspi, reinterpret_cast<const uint8_t*>(data.data()),
                                data.size())
           == HAL_OK;
  }

 protected:
  SpiImpl(Pinout pinout, ct::Frequency auto clock_frequency)
    requires(OM == SpiOperatingMode::Poll)
  {
    const auto presc = detail::FindPrescalerValue<Id, CS>(clock_frequency);

    detail::EnableSpiClk(Id);
    PinoutHelper::SetupPins(pinout);
    detail::SetupSpiMaster(Id, hspi, presc, DS, TT);
  }

  SpiImpl(hal::Dma auto& dma, Pinout pinout, ct::Frequency auto clock_frequency)
    requires(OM == SpiOperatingMode::Dma)
  {
    using DmaT = std::decay_t<decltype(dma)>;

    // Validate DMA instance
    static_assert(ct::Implies(hal::SpiTransmitEnabled(TT),
                              DmaT::template ChannelEnabled<TxDmaChannel>()),
                  "If the transmission mode allows transmission, the TX DMA "
                  "channel must be enabled for this core");
    static_assert(ct::Implies(hal::SpiReceiveEnabled(TT),
                              DmaT::template ChannelEnabled<RxDmaChannel>()),
                  "If the transmission mode allows transmission, the RX DMA "
                  "channel must be enabled for this core");

    detail::EnableSpiClk(Id);

    // Set up pins
    PinoutHelper::SetupPins(pinout);

    // Initialize SPI
    const auto presc = detail::FindPrescalerValue<Id, CS>(clock_frequency);
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
          hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
      __HAL_LINKDMA(&hspi, hdmarx, hrxdma);
    }

    // Enable interrupts
    detail::EnableSpiInterrupt(Id);
  }

  void RxComplete() noexcept {
    if constexpr (hal::AsyncRxSpiMaster<Impl>) {
      static_cast<Impl*>(this)->SpiReceiveCallback(rx_buf);
    }
  }

  void TxComplete() noexcept {
    if constexpr (hal::AsyncTxSpiMaster<Impl>) {
      static_cast<Impl*>(this)->SpiTransmitCallback();
    }
  }

 private:
  std::span<Data> rx_buf{};

  SPI_HandleTypeDef hspi{};
};

template <SpiId Id>
/**
 * Marker class for UART peripherals
 * @tparam Id UART id
 */
class Spi : public hal::UnusedPeripheral<Spi<Id>> {
  friend void ::HAL_SPI_TxCpltCallback(SPI_HandleTypeDef*);
  friend void ::HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);

 public:
  constexpr void HandleInterrupt() noexcept {}
  constexpr void RxComplete() noexcept {}
  constexpr void TxComplete() noexcept {}

 private:
  SPI_HandleTypeDef hspi{};
};

using Spi1 = Spi<SpiId::Spi1>;
using Spi2 = Spi<SpiId::Spi2>;
using Spi3 = Spi<SpiId::Spi3>;
using Spi4 = Spi<SpiId::Spi4>;
using Spi5 = Spi<SpiId::Spi5>;
using Spi6 = Spi<SpiId::Spi6>;

}   // namespace stm32h7