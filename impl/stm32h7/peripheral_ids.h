#pragma once

#include <string_view>
#include <utility>

#include <constexpr_tools/static_mapping.h>

#include <stm32h7xx.h>

namespace stm32h7 {

enum class SpiId {
  Spi1,
  Spi2,
  Spi3,
  Spi4,
  Spi5,
  Spi6,
};

[[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId spi) noexcept {
  switch (spi) {
  case SpiId::Spi1: return SPI1;
  case SpiId::Spi2: return SPI2;
  case SpiId::Spi3: return SPI3;
  case SpiId::Spi4: return SPI4;
  case SpiId::Spi5: return SPI5;
  case SpiId::Spi6: return SPI6;
  }

  std::unreachable();
}

[[nodiscard]] consteval SpiId SpiIdFromName(std::string_view name) noexcept {
  return ct::StaticMap<std::string_view, SpiId, 6>(name,
                                                   {{
                                                       {"SPI1", SpiId::Spi1},
                                                       {"SPI2", SpiId::Spi2},
                                                       {"SPI3", SpiId::Spi3},
                                                       {"SPI4", SpiId::Spi4},
                                                       {"SPI5", SpiId::Spi5},
                                                       {"SPI6", SpiId::Spi6},
                                                   }});
}

enum class I2sId {
  I2s1,
  I2s2,
  I2s3,
};

[[nodiscard]] consteval I2sId I2sIdFromName(std::string_view name) noexcept {
  return ct::StaticMap<std::string_view, I2sId, 3>(name,
                                                   {{
                                                       {"I2S1", I2sId::I2s1},
                                                       {"I2S2", I2sId::I2s2},
                                                       {"I2S3", I2sId::I2s3},
                                                   }});
}

}   // namespace stm32h7