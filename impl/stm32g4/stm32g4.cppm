module;

#include <stm32g4xx_hal.h>

export module hal.stm32g4;

export import :dma;
export import :nvic;
export import :clocks;
export import :peripherals;
export import :pin;
export import :system;
export import :uart;
export import :tim;
export import :tim.channel;

namespace stm32g4 {

export void InitializeHal() noexcept {
  HAL_Init();
}

}   // namespace stm32g4
