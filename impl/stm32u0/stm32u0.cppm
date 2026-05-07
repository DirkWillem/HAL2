module;

#include <stm32u0xx_hal.h>

export module hal.stm32u0;

export import :dma;
export import :pin;
export import :clocks;
export import :peripherals;
export import :uart;

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

/**
 * @brief Hook that is executed during an interrupt. Can be specialized to perform a hook on certain
 * interrupts.
 * @tparam IRQn IRQ number to overload the hook for.
 */
export template <IRQn_Type IRQn>
struct InterruptHook {
  static inline void operator()() {}
};

}   // namespace stm32u0
