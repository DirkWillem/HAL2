module;

#include <stm32u0xx_hal.h>

export module hal.stm32u0;

export import :pin;
export import :clocks;
export import :peripherals;

namespace stm32u0 {

/**
 * @brief Initializes the ST HAL.
 * @param enable_systick Whether to enable the SysTick interrupt at startup.
 */
export void InitializeHal(const bool enable_systick = true) {
  HAL_Init();

  if (!enable_systick) {
    NVIC_DisableIRQ(SysTick_IRQn);
  }
}

}   // namespace stm32u0
