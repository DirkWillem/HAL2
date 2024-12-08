#include "dma.h"

namespace stm32h5::detail {

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
  case hal::DmaMode::Circular: return DMA_LINKEDLIST_CIRCULAR;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalSrcDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_SRC_DATAWIDTH_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_SRC_DATAWIDTH_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_SRC_DATAWIDTH_WORD;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalDstDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_DEST_DATAWIDTH_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_DEST_DATAWIDTH_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_DEST_DATAWIDTH_WORD;
  }

  std::unreachable();
}

void SetupDma(std::size_t n_used_channels) noexcept {
#if defined(STM32H533xx)
  // Enable clocks
  if (n_used_channels > 0) {
    __HAL_RCC_GPDMA1_CLK_ENABLE();
  }

  if (n_used_channels > 8) {
    __HAL_RCC_GPDMA2_CLK_ENABLE();
  }

  // Enable IRQ
  static constexpr std::array<IRQn_Type, 16> ChannelIrqns{
      GPDMA1_Channel0_IRQn, GPDMA1_Channel1_IRQn, GPDMA1_Channel2_IRQn,
      GPDMA1_Channel3_IRQn, GPDMA1_Channel4_IRQn, GPDMA1_Channel5_IRQn,
      GPDMA1_Channel6_IRQn, GPDMA1_Channel7_IRQn, GPDMA2_Channel0_IRQn,
      GPDMA2_Channel1_IRQn, GPDMA2_Channel2_IRQn, GPDMA2_Channel3_IRQn,
      GPDMA2_Channel4_IRQn, GPDMA2_Channel5_IRQn, GPDMA2_Channel6_IRQn,
      GPDMA2_Channel7_IRQn,
  };

  for (std::size_t i = 0; i < n_used_channels; i++) {
    HAL_NVIC_SetPriority(ChannelIrqns[i], 0, 0);
    HAL_NVIC_EnableIRQ(ChannelIrqns[i]);
  }
#else
#error "Cannot determine DMA interrupts for this STM32H5 variant"
#endif
}

}   // namespace stm32h5::detail