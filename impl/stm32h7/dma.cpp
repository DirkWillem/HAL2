#include "dma.h"

namespace stm32h7::detail {

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

}   // namespace stm32h7::detail