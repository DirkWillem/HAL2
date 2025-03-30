#include "system.h"

#include <stm32g0xx.h>

namespace stm32g0 {

bool DisableIrqAtomicFlag::test_and_set() noexcept {
  __disable_irq();
  const bool ret = value;
  value          = true;
  __enable_irq();
  return ret;
}

void DisableIrqAtomicFlag::clear() {
  __disable_irq();
  value = false;
  __enable_irq();
}

void DisableInterruptsCriticalSectionInterface::Enter() noexcept {
  __disable_irq();
}

void DisableInterruptsCriticalSectionInterface::Exit() noexcept {
  __enable_irq();
}

}   // namespace stm32g0