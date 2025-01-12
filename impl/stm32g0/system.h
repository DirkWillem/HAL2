#pragma once

#include <atomic>

#include <hal/system.h>

namespace stm32g0 {

class CriticalSectionInterface {
 public:
  static void Enter() noexcept;
  static void Exit() noexcept;
};

struct BareMetalSystem {
  using CriticalSectionInterface = CriticalSectionInterface;

  template <typename T>
  using Atomic = std::atomic<T>;
};

static_assert(hal::System<BareMetalSystem>);

}   // namespace stm32g0