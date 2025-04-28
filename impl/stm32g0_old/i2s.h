#pragma once

#include <span>

#include <hal/dma.h>
#include <hal/i2s.h>

#include <stm32g0_old/dma.h>
#include <stm32g0_old/pin.h>
#include <stm32g0_old/spi.h>

#include <stm32g0_old/mappings/spi_i2s_pin_mapping.h>

namespace stm32g0 {

enum class I2sAudioFrequency : uint32_t {
  KHz8   = I2S_AUDIOFREQ_8K,
  KHz11  = I2S_AUDIOFREQ_11K,
  KHz16  = I2S_AUDIOFREQ_16K,
  KHz22  = I2S_AUDIOFREQ_22K,
  Khz32  = I2S_AUDIOFREQ_32K,
  KHz44  = I2S_AUDIOFREQ_44K,
  KHz48  = I2S_AUDIOFREQ_48K,
  KHz96  = I2S_AUDIOFREQ_96K,
  KHz192 = I2S_AUDIOFREQ_192K,
};

namespace detail {

template <I2sId Id, bool MCO>
struct I2sPinoutHelper {};

template <I2sId Id>
struct I2sPinoutHelper<Id, false> {
  struct Pinout {
    consteval Pinout(PinId ck, PinId sd, PinId ws,
                     hal::PinPull pull_ck = hal::PinPull::NoPull,
                     hal::PinPull pull_sd = hal::PinPull::NoPull,
                     hal::PinPull pull_ws = hal::PinPull::NoPull) noexcept
        : ck{ck}
        , sd{sd}
        , ws{ws}
        , pull_ck{pull_ck}
        , pull_sd{pull_sd}
        , pull_ws{pull_ws} {
      assert(("CK pin must be valid",
              hal::FindPinAFMapping(I2sCkPinMappings, Id, ck).has_value()));
      assert(("SD pin must be valid",
              hal::FindPinAFMapping(I2sSdPinMappings, Id, sd).has_value()));
      assert(("WS pin must be valid",
              hal::FindPinAFMapping(I2sWsPinMappings, Id, ws).has_value()));
    }

    PinId        ck;
    PinId        sd;
    PinId        ws;
    hal::PinPull pull_ck;
    hal::PinPull pull_sd;
    hal::PinPull pull_ws;
  };
};

void SetupI2s(I2S_HandleTypeDef& hi2s, I2sId id, I2sAudioFrequency frequency,
              hal::I2sStandard standard, hal::I2sDataFormat data_format,
              hal::I2sClockPolarity clock_polarity) noexcept;

}   // namespace detail

template <I2sId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2sTxDma = SpiTxDma<GetSpiForI2s(Id), Prio>;

template <typename Impl, I2sId Id, hal::I2sOperatingMode OM, bool MCO = false>
class I2sImpl : public hal::UsedPeripheral {
 public:
  using Pinout = detail::I2sPinoutHelper<Id, MCO>::Pinout;
  static constexpr auto OperatingMode = OM;

  using TxDmaChannel = DmaChannel<GetSpiForI2s(Id), SpiDmaRequest::Tx>;

  void Transmit(std::span<uint16_t> data)
    requires(OM == hal::I2sOperatingMode::Poll)
  {
    HAL_I2S_Transmit(&hi2s, data.data(), data.size(), HAL_MAX_DELAY);
  }

  void Transmit(std::span<uint16_t> data)
    requires(OM == hal::I2sOperatingMode::Dma)
  {
    HAL_I2S_Transmit_DMA(&hi2s, data.data(), data.size());
  }

  void StartTransmit(std::span<uint16_t> data)
    requires(OM == hal::I2sOperatingMode::DmaCircular)
  {
    HAL_I2S_Transmit_DMA(&hi2s, data.data(), data.size());
  }

  void EndTransmit(std::span<uint16_t>)
    requires(OM == hal::I2sOperatingMode::DmaCircular)
  {
    HAL_I2S_DMAStop(&hi2s);
  }

  /**
   * Singleton constructor
   * @return Singleton instance
   */
  [[nodiscard]] static Impl& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  void HandleInterrupt() { HAL_I2S_IRQHandler(&hi2s); }

 protected:
  I2sImpl(Pinout pinout, I2sAudioFrequency frequency,
          hal::I2sStandard   communication_standard = hal::I2sStandard::Philips,
          hal::I2sDataFormat data_format = hal::I2sDataFormat::Bits16,
          hal::I2sClockPolarity clock_polarity =
              hal::I2sClockPolarity::Low) noexcept
    requires(OM == hal::I2sOperatingMode::Poll)
  {
    // Set up pins
    Pin::InitializeAlternate(
        pinout.ck, hal::FindPinAFMapping(I2sCkPinMappings, Id, pinout.ck)->af,
        pinout.pull_ck);
    Pin::InitializeAlternate(
        pinout.sd, hal::FindPinAFMapping(I2sSdPinMappings, Id, pinout.sd)->af,
        pinout.pull_sd);
    Pin::InitializeAlternate(
        pinout.ws, hal::FindPinAFMapping(I2sWsPinMappings, Id, pinout.ws)->af,
        pinout.pull_ws);

    // Initialize I2S
    detail::SetupI2s(hi2s, Id, frequency, communication_standard, data_format,
                     clock_polarity);
  }

  I2sImpl(hal::Dma auto& dma, Pinout pinout, I2sAudioFrequency frequency,
          hal::I2sStandard   communication_standard = hal::I2sStandard::Philips,
          hal::I2sDataFormat data_format = hal::I2sDataFormat::Bits16,
          hal::I2sClockPolarity clock_polarity =
              hal::I2sClockPolarity::Low) noexcept
    requires(OM == hal::I2sOperatingMode::Dma
             || OM == hal::I2sOperatingMode::DmaCircular)
  {
    static_assert(dma.template ChannelEnabled<TxDmaChannel>(),
                  "TX DMA channel must be enabled");

    // Setup pins
    Pin::InitializeAlternate(
        pinout.ck, hal::FindPinAFMapping(I2sCkPinMappings, Id, pinout.ck)->af,
        pinout.pull_ck);
    Pin::InitializeAlternate(
        pinout.sd, hal::FindPinAFMapping(I2sSdPinMappings, Id, pinout.sd)->af,
        pinout.pull_sd);
    Pin::InitializeAlternate(
        pinout.ws, hal::FindPinAFMapping(I2sWsPinMappings, Id, pinout.ws)->af,
        pinout.pull_ws);

    // Initialize I2S
    detail::SetupI2s(hi2s, Id, frequency, communication_standard, data_format,
                     clock_polarity);

    // Set up DMA
    constexpr auto DmaMode = OM == hal::I2sOperatingMode::Dma
                                 ? hal::DmaMode::Normal
                                 : hal::DmaMode::Circular;
    auto&          htxdma  = dma.template SetupChannel<TxDmaChannel>(
        hal::DmaDirection::MemToPeriph, DmaMode, hal::DmaDataWidth::HalfWord,
        false, hal::DmaDataWidth::HalfWord, true);
    __HAL_LINKDMA(&hi2s, hdmatx, htxdma);
  }

  I2S_HandleTypeDef hi2s{};
};

template <I2sId Id>
class I2s : public hal::UnusedPeripheral<I2s<Id>> {
 public:
  constexpr void HandleInterrupt() noexcept {}
};

using I2s1 = I2s<I2sId::I2s1>;
using I2s2 = I2s<I2sId::I2s2>;

}   // namespace stm32g0
