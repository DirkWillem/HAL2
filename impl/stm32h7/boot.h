#ifndef STM32H7_CMAKE_TEST_BOOT_H
#define STM32H7_CMAKE_TEST_BOOT_H

#include <cstdint>
#include <optional>

#include <stm32h7xx_hal.h>

#include "core.h"
#include "hardware_semaphore.h"

namespace stm32h7 {

template <int Dummy = 0>
[[nodiscard]] bool WaitUntilCortexM4Ready(
    std::optional<uint32_t> timeout_cycles = std::nullopt) noexcept {
  uint32_t n_cycles = 0;
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET)) {
    if (timeout_cycles.has_value()) {
      n_cycles++;
      if (n_cycles >= *timeout_cycles) {
        return false;
      }
    }
  }

  return true;
}

template <uint32_t BootSemId>
[[nodiscard]] bool
WakeUpCortexM4(HardwareSemaphore<BootSemId>& hsem,
               std::optional<uint32_t> timeout_cycles = std::nullopt) noexcept requires (CurrentCore == Core::Cm7) {
  hsem.NotifyOtherCore();

  uint32_t n_cycles = 0;
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET)) {
    if (timeout_cycles.has_value()) {
      n_cycles++;
      if (n_cycles >= *timeout_cycles) {
        return false;
      }
    }
  }

  return true;
}

template <uint32_t BootSemId>
void WaitUntilWokenByCortexM7(HardwareSemaphore<BootSemId>& hsem) noexcept {
  hsem.EnableNotification();
  hsem.EnterStopModeUntilNotified();
}


}   // namespace stm32h7

#endif   // STM32H7_CMAKE_TEST_BOOT_H
