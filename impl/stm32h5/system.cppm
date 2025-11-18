module;

#include <atomic>

#include <stm32h5xx.h>

export module hal.stm32h5:system;

import hstd;
import hal.abstract;

import :clocks;

namespace stm32h5 {

export class DisableInterruptsCriticalSectionInterface {
 public:
  static void Enter() noexcept { __disable_irq(); }
  static void Exit() noexcept { __enable_irq(); }
};

export struct BareMetalSystem {
  using CriticalSectionInterface = DisableInterruptsCriticalSectionInterface;
  using Clock                    = SysTickClock;

  template <typename T>
  using Atomic = std::atomic<T>;

  using AtomicFlag = std::atomic_flag;
};

/**
 * @brief Helper struct for the performance timer.
 */
export struct PerformanceTimer {
  /**
   * @brief Enables the performance timer.
   */
  static void Enable() noexcept {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  }

  /**
   * @brief Returns the current amount of cycles, usable for determining
   * performance.
   *
   * @return Current cycle count.
   */
  static uint32_t Get() noexcept { return DWT->CYCCNT; }
};

}   // namespace stm32h5
