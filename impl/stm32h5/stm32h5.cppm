module;

#include <stm32h5xx_hal.h>

export module hal.stm32h5;

export import :clocks;
export import :dma;
export import :peripherals;
export import :pin;
export import :system;
export import :uart;

namespace stm32h5 {

/**
 * @brief Initializes the ST HAL.
 *
 * @param enable_systick Whether to enable the SysTick interrupt at startup.
 */
export void InitializeHal(const bool enable_systick = true) noexcept {
  HAL_Init();

  if (!enable_systick) {
    NVIC_DisableIRQ(SysTick_IRQn);
  }
}

/**
 * @brief Disables the SysTick interrupt.
 */
export void DisableSysTick() {
  SysTick->CTRL = 0;
  NVIC_ClearPendingIRQ(SysTick_IRQn);
  NVIC_DisableIRQ(SysTick_IRQn);
}

/**
 * @brief Enables the SysTick interrupt with a given priority.
 *
 * @param tick_prio SysTick interrupt priority.
 */
export void EnableSysTick(const uint32_t tick_prio = TICK_INT_PRIORITY) {
  HAL_InitTick(tick_prio);
}

}   // namespace stm32h5