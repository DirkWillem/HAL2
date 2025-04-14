export module hal.abstract:i2s;

namespace hal {

/**
 * I2S communication standard setting
 */
export enum class I2sStandard {
  Philips,
  LeftJustified,
  RightJustified,
  PcmShortSynchroFrame,
  PcmLongSynchroFrame,
};

/**
 * I2S data format
 */
export enum class I2sDataFormat {
  Bits16,
  Bits16On32BitFrame,
  Bits24On32BitFrame,
  Bits32
};

/**
 * Operating modes for I2S
 */
export enum class I2sOperatingMode { Poll, Dma, DmaCircular };

/**
 * I2S clock polarity
 */
export enum class I2sClockPolarity { Low, High };

}   // namespace hal