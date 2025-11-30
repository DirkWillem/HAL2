module;

#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <stm32h5xx.h>
#include <stm32h5xx_hal_conf.h>
#include <stm32h5xx_hal_i2s.h>

export module hal.stm32h5:i2s;

import hal.abstract;

import :clocks;
import :dma;
import :peripherals;
import :spi;

export import :i2s.config;
export import :i2s.pinout;

extern "C" {

[[maybe_unused]] void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef* hi2s);
[[maybe_unused]] void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef* hi2s);
[[maybe_unused]] void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s);
[[maybe_unused]] void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s);
}

namespace stm32h5 {

export template <I2sId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2sTxDma = SpiTxDma<GetSpiForI2s(Id), Prio>;

export template <I2sId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2sRxDma = SpiRxDma<GetSpiForI2s(Id), Prio>;

template <I2sOperatingMode OM, typename... Ts>
class I2sImplBase {
 protected:
  using EG = int;
};

template <>
class I2sImplBase<I2sOperatingMode::Dma> {
 protected:
  using EG = int;

  DMA_HandleTypeDef* hdma_tx;
  DMA_HandleTypeDef* hdma_rx;
};

template <typename OS>
class I2sImplBase<I2sOperatingMode::DmaRtos, OS> {
 protected:
  using EG = OS::EventGroup;
  std::tuple<EG*, uint32_t, uint32_t> tx_event_group{};
  std::tuple<EG*, uint32_t, uint32_t> rx_event_group{};

  DMA_HandleTypeDef* hdma_tx;
  DMA_HandleTypeDef* hdma_rx;
};

export template <typename Impl, I2sId Id, ClockSettings CS, I2sSettings IS,
                 typename... Rest>
class I2sImpl
    : public hal::UsedPeripheral
    , I2sImplBase<IS.operating_mode, Rest...> {
  using PinoutHelper          = I2sPinoutHelper<Id, IS>;
  static constexpr auto SpiId = GetSpiForI2s(Id);

 public:
  friend void ::HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef* hi2s);
  friend void ::HAL_I2S_RxCpltCallback(I2S_HandleTypeDef* hi2s);
  friend void ::HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s);
  friend void ::HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s);

  using TxDmaChannel = I2sTxDma<Id>;   //!< Transmit DMA channel for this I2S.
  using RxDmaChannel = I2sRxDma<Id>;   //!< Receive DMA channel for this I2S.

  using Pinout = PinoutHelper::Pinout;   //!< Pinout type.

  void HandleInterrupt() { HAL_I2S_IRQHandler(&hi2s); }

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
   * @brief Transmits I2S data.
   *
   * @param data Data to transmit.
   */
  void Transmit(std::span<const uint16_t> data) noexcept
    requires((IS.transmit_mode == I2sTransmitMode::Tx
              || IS.transmit_mode == I2sTransmitMode::Duplex)
             && IS.operating_mode == I2sOperatingMode::DmaRtos)
  {
    HAL_I2S_Transmit_DMA(&hi2s, data.data(), data.size());
  }

  /**
   * @brief Receives I2S data.
   *
   * @param into Buffer to receive data into.
   * @param event_group Event group in which a bit is set upon receival.
   * @param half_bit Bit that should be set when the buffer is half full.
   * @param full_bit Bit that should be set when the buffer is full.
   */
  void Receive(std::span<uint16_t>                          into,
               I2sImplBase<IS.operating_mode, Rest...>::EG& event_group,
               uint32_t half_bit, uint32_t full_bit) noexcept
    requires((IS.transmit_mode == I2sTransmitMode::Rx
              || IS.transmit_mode == I2sTransmitMode::Duplex)
             && IS.operating_mode == I2sOperatingMode::DmaRtos)
  {
    I2sImplBase<IS.operating_mode, Rest...>::rx_event_group =
        std::make_tuple(&event_group, half_bit, full_bit);

    HAL_I2S_Receive_DMA(&hi2s, into.data(), into.size() / 2);
  }

 protected:
  I2sImpl(hal::Dma auto& dma, Pinout pinout) noexcept
    requires(IS.operating_mode == I2sOperatingMode::Dma
             || IS.operating_mode == I2sOperatingMode::DmaRtos)
  {
    // Set up pins, source clock and I2S master
    PinoutHelper::SetupPins(pinout);
    if (!SetupSourceClock<SpiId, IS.source_clock>()) {
      return;
    }
    if (!InitializePeripheral()) {
      return;
    }

    // Set up DMA channels
    if constexpr (IS.transmit_mode == I2sTransmitMode::Tx
                  || IS.transmit_mode == I2sTransmitMode::Duplex) {
      auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
          hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
          hal::DmaDataWidth::HalfWord, false, hal::DmaDataWidth::HalfWord,
          true);
      __HAL_LINKDMA(&hi2s, hdmatx, htxdma);
    }
    if constexpr (IS.transmit_mode == I2sTransmitMode::Rx
                  || IS.transmit_mode == I2sTransmitMode::Duplex) {
      auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
          hal::DmaDirection::PeriphToMem, hal::DmaMode::Circular,
          hal::DmaDataWidth::Word, false, hal::DmaDataWidth::Word, true);
      __HAL_LINKDMA(&hi2s, hdmarx, hrxdma);
    }

    // Enable interrupt
    EnableSpiInterrupt<SpiId, Impl>();
  }

  /** @brief Receive half complete callback. */
  void HalfReceiveComplete() noexcept {
    if constexpr (IS.operating_mode == I2sOperatingMode::DmaRtos) {
      auto& [eg, hbit, fbit] =
          I2sImplBase<IS.operating_mode, Rest...>::rx_event_group;
      eg->SetBitsFromInterrupt(hbit);
    }
  }

  /** @brief Receive complete callback. */
  void ReceiveComplete() noexcept {
    if constexpr (IS.operating_mode == I2sOperatingMode::DmaRtos) {
      auto& [eg, hbit, fbit] =
          I2sImplBase<IS.operating_mode, Rest...>::rx_event_group;
      eg->SetBitsFromInterrupt(fbit);
    }
  }

  /** @brief Transmit half complete callback. */
  void HalfTransmitComplete() noexcept {
    if constexpr (IS.operating_mode == I2sOperatingMode::DmaRtos) {
      auto& [eg, hbit, fbit] =
          I2sImplBase<IS.operating_mode, Rest...>::tx_event_group;
      eg->SetBitsFromInterrupt(hbit);
    }
  }

  /** @brief Transmit complete callback. */
  void TransmitComplete() noexcept {
    if constexpr (IS.operating_mode == I2sOperatingMode::DmaRtos) {
      auto& [eg, hbit, fbit] =
          I2sImplBase<IS.operating_mode, Rest...>::tx_event_group;
      eg->SetBitsFromInterrupt(fbit);
    }
  }

  I2S_HandleTypeDef hi2s;

 private:
  bool InitializePeripheral() noexcept {
    EnableSpiClock<SpiId>();

    hi2s.Instance = GetSpiPointer(SpiId);
    hi2s.Init     = {
            .Mode       = GetHalMode(),
            .Standard   = static_cast<uint32_t>(IS.standard),
            .DataFormat = static_cast<uint32_t>(IS.data_format),
            .MCLKOutput = IS.master_clock_output ? I2S_MCLKOUTPUT_ENABLE
                                                 : I2S_MCLKOUTPUT_DISABLE,
            .AudioFreq  = static_cast<uint32_t>(
            IS.audio_frequency.template As<hstd::Hz>().count),
            .CPOL               = static_cast<uint32_t>(IS.clock_polarity),
            .FirstBit           = static_cast<uint32_t>(IS.bit_order),
            .WSInversion        = static_cast<uint32_t>(IS.ws_inversion),
            .Data24BitAlignment = static_cast<uint32_t>(IS.data_24bit_alignment),
            .MasterKeepIOState  = static_cast<uint32_t>(IS.keep_io_state),
    };

    return HAL_I2S_Init(&hi2s) == HAL_OK;
  }

  static constexpr uint32_t GetHalMode() {
    using enum I2sMode;
    using enum I2sTransmitMode;

    switch (IS.mode) {
    case Master:
      switch (IS.transmit_mode) {
      case Tx: return I2S_MODE_MASTER_TX;
      case Rx: return I2S_MODE_MASTER_RX;
      case Duplex: return I2S_MODE_MASTER_FULLDUPLEX;
      }
    case Slave:
      switch (IS.transmit_mode) {
      case Tx: return I2S_MODE_SLAVE_TX;
      case Rx: return I2S_MODE_SLAVE_RX;
      case Duplex: return I2S_MODE_SLAVE_FULLDUPLEX;
      }
    }

    std::unreachable();
  }
};

export template <I2sId Id>
/**
 * Marker class for I2S peripherals
 * @tparam Id I2S id
 */
class I2s : public hal::UnusedPeripheral<I2s<Id>> {
 public:
  friend void ::HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef* hi2s);
  friend void ::HAL_I2S_RxCpltCallback(I2S_HandleTypeDef* hi2s);
  friend void ::HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s);
  friend void ::HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s);

  constexpr void HandleInterrupt() noexcept {}

 protected:
  void              HalfReceiveComplete() noexcept {}
  void              ReceiveComplete() noexcept {}
  void              HalfTransmitComplete() noexcept {}
  void              TransmitComplete() noexcept {}
  I2S_HandleTypeDef hi2s{};
};

export using I2s1 = I2s<I2sId::I2s1>;
export using I2s2 = I2s<I2sId::I2s2>;
export using I2s3 = I2s<I2sId::I2s3>;

}   // namespace stm32h5
