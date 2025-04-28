#include "system.h"

#include <stm32g0xx.h>

namespace stm32g0 {

bool DisableIrqAtomicFlag::test() const noexcept {
  __disable_irq();
  __DMB();
  const auto ret = value;
  __DMB();
  __enable_irq();
  return ret;
}


bool DisableIrqAtomicFlag::test_and_set() noexcept {
  __disable_irq();
  __DMB();
  const bool ret = value;
  value          = true;
  __DMB();
  __enable_irq();
  return ret;
}

void DisableIrqAtomicFlag::clear() {
  __disable_irq();
  __DMB();
  value = false;
  __DMB();
  __enable_irq();
}

void DisableInterruptsCriticalSectionInterface::Enter() noexcept {
  __disable_irq();
}

void DisableInterruptsCriticalSectionInterface::Exit() noexcept {
  __enable_irq();
}

}   // namespace stm32g0