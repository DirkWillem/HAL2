module;

#include <stm32g0xx_hal.h>

export module hal.stm32g0;

export import :dma;
export import :clocks;
export import :pin;
export import :pin_interrupt;
export import :peripherals;
export import :system;
export import :uart;
export import :tim;

namespace stm32g0 {

export void InitializeHal(bool enable_systick = true) noexcept {
  HAL_Init();

  if (!enable_systick) {
    NVIC_DisableIRQ(SysTick_IRQn);
  }
}

export void DisableSysTick() {
  SysTick->CTRL = 0;
  NVIC_ClearPendingIRQ(SysTick_IRQn);
  NVIC_DisableIRQ(SysTick_IRQn);
}

export void EnableSysTick(uint32_t tick_prio = TICK_INT_PRIORITY) {
  HAL_InitTick(tick_prio);
}

}   // namespace stm32g0