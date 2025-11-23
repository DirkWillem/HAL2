module;

#include "stm32h5xx_hal_spi.h"

#include <concepts>
#include <cstdint>

export module hal.stm32h5:spi.config;

import hstd;

import hal.abstract;

namespace stm32h5 {

export enum class SpiOperatingMode : uint8_t { Poll, Dma, DmaRtos };

export enum class SpiSourceClock : uint8_t {
  // Available on SPI1/2/3
  Pll1Q,      //!< PLL 1 Q
  Pll2P,      //!< PLL 2 P
  Pll3P,      //!< PLL 3 P
  AudioClk,   //!< Audio clock
  // Available on SPI4
  Pclk2,   //!< Peripheral clock 2
  Pll2Q,   //!< PLL 2 Q
  Hsi,     //!< HSI clock
  Csi,     //!< CSI clock
  Hse,     //!< HSE clock
  // Available on all SPI instances
  Per,   //!< PER clock

};

export enum class SpiBitOrder : uint32_t {
  MsbFirst = SPI_FIRSTBIT_MSB,   //!< Most Significant Bit first.
  LsbFirst = SPI_FIRSTBIT_LSB,   //!< Least Significant Bit first.
};

/**
 * @brief Configuration of a SPI peripheral.
 * @tparam F Frequency type.
 */
export template <typename F = hstd::Hz>
struct SpiSettings {
  SpiSourceClock           source_clock;     //!< SPI source clock
  F                        frequency;        //!< SPI Frequency
  SpiOperatingMode         operating_mode;   //!< SPI operating mode
  hal::SpiMode             mode = hal::SpiMode::Master;   //!< SPI mode
  hal::SpiTransmissionType transmission_type;        //!< SPI transmission type
  unsigned                 data_size = 8;            //!< SPI data size
  SpiBitOrder bit_order   = SpiBitOrder::MsbFirst;   //!< SPI bit order
  bool        hardware_cs = false;                   //!< Hardware Chip Select
};

}   // namespace stm32h5