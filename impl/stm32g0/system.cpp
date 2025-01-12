#include "system.h"

#include <stm32g0xx.h>

namespace stm32g0 {

void CriticalSectionInterface::Enter() noexcept {
  __disable_irq();
}

void CriticalSectionInterface::Exit() noexcept {
  __enable_irq();
}

}   // namespace stm32g4