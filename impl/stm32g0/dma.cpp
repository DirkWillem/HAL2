#include "dma.h"

namespace stm32g0::detail {

[[nodiscard]] uint32_t GetDmaRequestId(UartId         id,
                                       UartDmaRequest request) noexcept {
  switch (request) {
  case UartDmaRequest::Tx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_TX;
    case UartId::Usart2: return DMA_REQUEST_USART2_TX;
    case UartId::Usart3: return DMA_REQUEST_USART3_TX;
    case UartId::Usart4: return DMA_REQUEST_USART4_TX;
    case UartId::Usart5: return DMA_REQUEST_USART5_TX;
    case UartId::Usart6: return DMA_REQUEST_USART6_TX;
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_TX;
    case UartId::LpUart2: return DMA_REQUEST_LPUART2_TX;
    }
    std::unreachable();
  case UartDmaRequest::Rx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_RX;
    case UartId::Usart2: return DMA_REQUEST_USART2_RX;
    case UartId::Usart3: return DMA_REQUEST_USART3_RX;
    case UartId::Usart4: return DMA_REQUEST_USART4_RX;
    case UartId::Usart5: return DMA_REQUEST_USART5_RX;
    case UartId::Usart6: return DMA_REQUEST_USART6_RX;
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_RX;
    case UartId::LpUart2: return DMA_REQUEST_LPUART2_RX;
    }
    std::unreachable();
  }
  std::unreachable();
}

[[nodiscard]] uint32_t GetDmaRequestId(SpiId         id,
                                       SpiDmaRequest request) noexcept {
  switch (request) {
  case SpiDmaRequest::Tx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_TX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_TX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_TX;
    }
    std::unreachable();
  case SpiDmaRequest::Rx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_RX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_RX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_RX;
    }
    std::unreachable();
  }
  std::unreachable();
}

[[nodiscard]] uint32_t GetDmaRequestId(TimId         id,
                                       TimDmaRequest request) noexcept {
  switch (id) {
  case TimId::Tim1:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM1_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM1_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM1_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM1_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM1_CH4;
    default: std::unreachable();
    }
  case TimId::Tim2:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM2_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM2_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM2_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM2_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM2_CH4;
    default: std::unreachable();
    }
  case TimId::Tim3:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM3_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM3_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM3_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM3_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM3_CH4;
    default: std::unreachable();
    }
  case TimId::Tim4:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM4_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM4_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM4_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM4_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM4_CH4;
    default: std::unreachable();
    }
  case TimId::Tim15:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM15_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM15_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM15_CH2;
    default: std::unreachable();
    }
  case TimId::Tim16:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM16_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM16_CH1;
    default: std::unreachable();
    }
  case TimId::Tim17:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM17_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM17_CH1;
    default: std::unreachable();
    }
  }

  std::unreachable();
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
#if defined(STM32G0B1xx) || defined(STM32G0C1xx)
  // Enable clocks
  if (n_used_channels > 0) {
    __HAL_RCC_DMA1_CLK_ENABLE();
  }

  if (n_used_channels > 7) {
    __HAL_RCC_DMA2_CLK_ENABLE();
  }

  // Enable IRQ
  if (n_used_channels > 0) {
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  }
  if (n_used_channels > 1) {
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
  }
  if (n_used_channels > 3) {
    HAL_NVIC_SetPriority(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQn);
  }
#else
#error "Cannot determine DMA interrupts for this STM32G0 variant"
#endif
}

}   // namespace stm32g0::detail