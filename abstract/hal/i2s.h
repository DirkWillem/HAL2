#pragma once

namespace hal {

/**
 * I2S communication standard setting
 */
enum class I2sStandard {
  Philips,
  LeftJustified,
  RightJustified,
  PcmShortSynchroFrame,
  PcmLongSynchroFrame,
};

/**
 * I2S data format
 */
enum class I2sDataFormat {
  Bits16,
  Bits16On32BitFrame,
  Bits24On32BitFrame,
  Bits32
};

/**
 * Operating modes for I2S
 */
enum class I2sOperatingMode { Poll, Dma, DmaCircular };

/**
 * I2S clock polarity
 */
enum class I2sClockPolarity { Low, High };

}   // namespace hal