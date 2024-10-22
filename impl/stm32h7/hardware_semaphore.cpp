#include "hardware_semaphore.h"

#include "core.h"

namespace stm32h7::detail {

void InitializeHsemInterrupt() noexcept {
  __HAL_RCC_HSEM_CLK_ENABLE();

  if constexpr (CurrentCore == Core::Cm7) {
    HAL_NVIC_SetPriority(HSEM1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(HSEM1_IRQn);
  } else if constexpr (CurrentCore == Core::Cm4) {
    HAL_NVIC_SetPriority(HSEM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(HSEM2_IRQn);
  }
}

}   // namespace stm32h7::detail
