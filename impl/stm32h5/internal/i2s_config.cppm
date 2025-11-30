module;

#include <stm32h5xx_hal.h>

export module hal.stm32h5:i2s.config;

import :spi.config;

namespace stm32h5 {

export enum class I2sOperatingMode : uint8_t {
  Dma,
  DmaRtos,
};

export enum class I2sMode : uint8_t { Master, Slave };

export enum class I2sTransmitMode : uint8_t { Duplex, Tx, Rx };

export enum class I2sCommunicationStandard : uint32_t {
  Philips         = I2S_STANDARD_PHILIPS,
  MsbFirst        = I2S_STANDARD_MSB,
  LsbFirst        = I2S_STANDARD_LSB,
  PcmShortSynchro = I2S_STANDARD_PCM_SHORT,
  PcmLongSynchro  = I2S_STANDARD_PCM_LONG,
};

export enum class I2sDataFormat : uint32_t {
  Bits16Frame16 = I2S_DATAFORMAT_16B,   //!< 16 bits data on 16 bit frame.
  Bits16Frame32 =
      I2S_DATAFORMAT_16B_EXTENDED,      //!< 16 bits data on 32 bit frame.
  Bits24Frame32 = I2S_DATAFORMAT_24B,   //!< 24 bits data on 32 bit frame.
  Bits32Frame32 = I2S_DATAFORMAT_32B,   //!< 32 bits data on 32 bit frame.
};

export enum class I2sClockPolarity : uint32_t {
  Low  = I2S_CPOL_LOW,    //!< Low clock polarity.
  High = I2S_CPOL_HIGH,   //!< High clock polarity.
};

export enum class I2sBitOrder : uint32_t {
  Msb = I2S_FIRSTBIT_MSB,   //!< MSB first.
  Lsb = I2S_FIRSTBIT_LSB,   //!< LSB first.
};

export enum class I2sWsInversion : uint32_t {
  Disable = I2S_WS_INVERSION_DISABLE,   //!< Disable inversion.
  Enable  = I2S_WS_INVERSION_ENABLE,    //!< Enable inversion.
};

export enum class I2sData24BitAlignment : uint32_t {
  Right = I2S_DATA_24BIT_ALIGNMENT_RIGHT,   //!< Align right.
  Left  = I2S_DATA_24BIT_ALIGNMENT_LEFT,    //!< Align left
};

export enum class I2sMasterKeepIoState : uint32_t {
  Disable = I2S_MASTER_KEEP_IO_STATE_DISABLE,
  Enable  = I2S_MASTER_KEEP_IO_STATE_ENABLE,
};

export template <typename F = hstd::Hz>
struct I2sSettings {
  I2sOperatingMode operating_mode;

  SpiSourceClock source_clock;

  F audio_frequency;

  bool                     master_clock_output = false;
  I2sMode                  mode                = I2sMode::Master;
  I2sTransmitMode          transmit_mode       = I2sTransmitMode::Duplex;
  I2sCommunicationStandard standard       = I2sCommunicationStandard::Philips;
  I2sDataFormat            data_format    = I2sDataFormat::Bits16Frame16;
  I2sClockPolarity         clock_polarity = I2sClockPolarity::Low;
  I2sBitOrder              bit_order      = I2sBitOrder::Msb;
  I2sWsInversion           ws_inversion   = I2sWsInversion::Disable;
  I2sData24BitAlignment    data_24bit_alignment = I2sData24BitAlignment::Left;
  I2sMasterKeepIoState     keep_io_state        = I2sMasterKeepIoState::Disable;
};

}   // namespace stm32h5