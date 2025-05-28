#include "system.h"

#include <stm32g4xx.h>

namespace stm32g4 {

void CriticalSectionInterface::Enter() noexcept {
  __disable_irq();
}

void CriticalSectionInterface::Exit() noexcept {
  __enable_irq();
}

}   // namespace stm32g4