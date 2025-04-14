module;

#include <atomic>

#include <stm32g0xx.h>

export module hal.stm32g0:system;

import hstd;
import hal.abstract;

import :clocks;

namespace stm32g0 {

export class DisableIrqAtomicFlag {
 public:
  bool test() const noexcept {
    __disable_irq();
    __DMB();
    const auto ret = value;
    __DMB();
    __enable_irq();
    return ret;
  }

  bool test_and_set() noexcept {
    __disable_irq();
    __DMB();
    const bool ret = value;
    value          = true;
    __DMB();
    __enable_irq();
    return ret;
  }

  void clear() {
    __disable_irq();
    __DMB();
    value = false;
    __DMB();
    __enable_irq();
  }

 private:
  volatile bool value{false};
};

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

  using AtomicFlag = DisableIrqAtomicFlag;
};

static_assert(hal::System<BareMetalSystem>);

}   // namespace stm32g0