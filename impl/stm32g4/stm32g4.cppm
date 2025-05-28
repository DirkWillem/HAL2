module;

#include <stm32g4xx_hal.h>

export module hal.stm32g4;

export import :clocks;
export import :pin;
export import :system;

namespace stm32g4 {

export void InitializeHal() noexcept {
  HAL_Init();
}

}   // namespace stm32g4
