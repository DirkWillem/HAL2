module;

#include <atomic>

#include <stm32g4xx.h>

export module hal.stm32g4:system;

import hstd;
import hal.abstract;

import :clocks;

namespace stm32g4 {

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

}   // namespace stm32g4
