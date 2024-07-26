#include "dma.h"

namespace stm32g4::detail {

[[nodiscard]] uint32_t GetDmaRequestId(UartId         id,
                                       UartDmaRequest request) noexcept {
  switch (request) {
  case UartDmaRequest::Tx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_TX;
    case UartId::Usart2: return DMA_REQUEST_USART2_TX;
    case UartId::Usart3: return DMA_REQUEST_USART3_TX;
    case UartId::Uart4: return DMA_REQUEST_UART4_TX;
    case UartId::Uart5: return DMA_REQUEST_UART5_TX;
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_TX;
    }
    std::unreachable();
  case UartDmaRequest::Rx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_RX;
    case UartId::Usart2: return DMA_REQUEST_USART2_RX;
    case UartId::Usart3: return DMA_REQUEST_USART3_RX;
    case UartId::Uart4: return DMA_REQUEST_UART4_RX;
    case UartId::Uart5: return DMA_REQUEST_UART5_RX;
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_RX;
    }
    std::unreachable();
  }
  std::unreachable();
}

[[nodiscard]] uint32_t GetDmaRequestId(I2cId         id,
                                       I2cDmaRequest request) noexcept {
  switch (request) {
  case I2cDmaRequest::Tx:
    switch (id) {
    case I2cId::I2c1: return DMA_REQUEST_I2C1_TX;
    case I2cId::I2c2: return DMA_REQUEST_I2C2_TX;
    case I2cId::I2c3: return DMA_REQUEST_I2C3_TX;
    case I2cId::I2c4: return DMA_REQUEST_I2C4_TX;
    default: std::unreachable();
    }
  case I2cDmaRequest::Rx:
    switch (id) {
    case I2cId::I2c1: return DMA_REQUEST_I2C1_RX;
    case I2cId::I2c2: return DMA_REQUEST_I2C2_RX;
    case I2cId::I2c3: return DMA_REQUEST_I2C3_RX;
    case I2cId::I2c4: return DMA_REQUEST_I2C4_RX;
    default: std::unreachable();
    }
  default: std::unreachable();
  }
}


[[nodiscard]] uint32_t GetDmaRequestId(SpiId         id,
                                       SpiDmaRequest request) noexcept {
  switch (request) {
  case SpiDmaRequest::Tx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_TX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_TX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_TX;
    default: std::unreachable();
    }
  case SpiDmaRequest::Rx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_RX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_RX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_RX;
    case SpiId::Spi4: return DMA_REQUEST_SPI4_RX;
    default: std::unreachable();
    }
  default: std::unreachable();
  }
}

[[nodiscard]] uint32_t ToHalDmaDirection(hal::DmaDirection dir) noexcept {
  switch (dir) {
  case hal::DmaDirection::MemToPeriph: return DMA_MEMORY_TO_PERIPH;
  case hal::DmaDirection::PeriphToMem: return DMA_PERIPH_TO_MEMORY;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t ToHalDmaMode(hal::DmaMode mode) noexcept {
  switch (mode) {
  case hal::DmaMode::Normal: return DMA_NORMAL;
  case hal::DmaMode::Circular: return DMA_CIRCULAR;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalMemDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_MDATAALIGN_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_MDATAALIGN_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_MDATAALIGN_WORD;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalPeriphDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_PDATAALIGN_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_PDATAALIGN_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_PDATAALIGN_WORD;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t ToHalDmaPriority(hal::DmaPriority prio) {
  switch (prio) {
  case hal::DmaPriority::Low: return DMA_PRIORITY_LOW;
  case hal::DmaPriority::Medium: return DMA_PRIORITY_MEDIUM;
  case hal::DmaPriority::High: return DMA_PRIORITY_HIGH;
  case hal::DmaPriority::VeryHigh: return DMA_PRIORITY_VERY_HIGH;
  }

  std::unreachable();
}

void SetupDma(std::size_t n_used_channels) noexcept {
#if defined(STM32G474xx)
  // Enable clocks
  if (n_used_channels > 0) {
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
  }

  if (n_used_channels > 8) {
    __HAL_RCC_DMA2_CLK_ENABLE();
  }

  // Enable IRQ
  static constexpr std::array<IRQn_Type, 16> ChannelIrqns{
      DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn,
      DMA1_Channel4_IRQn, DMA1_Channel5_IRQn, DMA1_Channel6_IRQn,
      DMA1_Channel7_IRQn, DMA1_Channel8_IRQn, DMA2_Channel1_IRQn,
      DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn,
      DMA2_Channel5_IRQn, DMA2_Channel6_IRQn, DMA2_Channel7_IRQn,
      DMA2_Channel8_IRQn,
  };

  for (std::size_t i = 0; i < n_used_channels; i++) {
    HAL_NVIC_SetPriority(ChannelIrqns[i], 0, 0);
    HAL_NVIC_EnableIRQ(ChannelIrqns[i]);
  }
#else
#error "Cannot determine DMA interrupts for this STM32G4 variant"
#endif
}

}   // namespace stm32g4::detail