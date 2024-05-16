#include "spi.h"

namespace stm32g0::detail {

void EnableSpiClk(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: __HAL_RCC_SPI1_CLK_ENABLE(); break;
  case SpiId::Spi2: __HAL_RCC_SPI2_CLK_ENABLE(); break;
  case SpiId::Spi3: __HAL_RCC_SPI3_CLK_ENABLE(); break;
  }
}

void EnableSpiInterrupt(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1:
    HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    break;
  case SpiId::Spi2: [[fallthrough]];
  case SpiId::Spi3:
    HAL_NVIC_SetPriority(SPI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI2_3_IRQn);
    break;
  }
}

}   // namespace stm32g0::detail