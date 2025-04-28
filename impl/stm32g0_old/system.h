#pragma once

#include <atomic>

#include <hal/system.h>

#include "clocks.h"

namespace stm32g0 {

class DisableIrqAtomicFlag {
public:
  bool test() const noexcept;
  bool test_and_set() noexcept;
  void clear();

private:
  volatile bool value{false};
};

class DisableInterruptsCriticalSectionInterface {
 public:
  static void Enter() noexcept;
  static void Exit() noexcept;
};

struct BareMetalSystem {
  using CriticalSectionInterface = DisableInterruptsCriticalSectionInterface;
  using Clock                    = SysTickClock;

  template <typename T>
  using Atomic = std::atomic<T>;

  using AtomicFlag = DisableIrqAtomicFlag;
};

static_assert(hal::System<BareMetalSystem>);

}   // namespace stm32g0