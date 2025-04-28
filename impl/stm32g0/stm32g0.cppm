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

export void InitializeHal() noexcept {
  HAL_Init();
}

}