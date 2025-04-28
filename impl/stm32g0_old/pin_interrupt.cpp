#include "pin_interrupt.h"

namespace stm32g0::detail {

[[nodiscard]] constexpr IRQn_Type GetExtiIrqNumber(PinNum pin_num) noexcept {
  switch (pin_num) {
  case 0: [[fallthrough]];
  case 1: return EXTI0_1_IRQn;
  case 2: [[fallthrough]];
  case 3: return EXTI2_3_IRQn;
  case 4: [[fallthrough]];
  case 5: [[fallthrough]];
  case 6: [[fallthrough]];
  case 7: [[fallthrough]];
  case 8: [[fallthrough]];
  case 9: [[fallthrough]];
  case 10: [[fallthrough]];
  case 11: [[fallthrough]];
  case 12: [[fallthrough]];
  case 13: [[fallthrough]];
  case 14: [[fallthrough]];
  case 15: return EXTI4_15_IRQn;
  default: std::unreachable();
  }
}

void EnableExtiInterrupt(PinId pin) noexcept {
  const auto irqn = GetExtiIrqNumber(pin.num);

  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);
}

}   // namespace stm32g0::detail