#pragma once

#include <atomic>

#include <hal/system.h>

#include "clocks.h"

namespace stm32g0 {

class CriticalSectionInterface {
 public:
  static void Enter() noexcept;
  static void Exit() noexcept;
};

struct BareMetalSystem {
  using CriticalSectionInterface = CriticalSectionInterface;
  using Clock                    = SysTickClock;

  template <typename T>
  using Atomic = std::atomic<T>;

  using AtomicFlag = std::atomic_flag;
};

static_assert(hal::System<BareMetalSystem>);

}   // namespace stm32g0