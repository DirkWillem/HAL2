module;

#include <atomic>
#include <chrono>

#include <FreeRTOS.h>

#include <task.h>

export module rtos.freertos:system;

import :time;

namespace rtos {

export class OsCriticalSectionInterface {
 public:
  static void Enter() noexcept { taskENTER_CRITICAL(); }
  static void Exit() noexcept { taskEXIT_CRITICAL(); }
};

export class OsClock {
 public:
  using rep        = uint32_t;
  using period     = std::milli;
  using duration   = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<OsClock, duration>;

  static constexpr auto is_steady = false;

  [[nodiscard]] static time_point now() {
    return time_point{TicksToMs(xTaskGetTickCount())};
  }

  static void BlockFor(hstd::Duration auto duration) noexcept {
    vTaskDelay(ToTicks(duration));
  }
};

export struct System {
  using CriticalSectionInterface = OsCriticalSectionInterface;
  using Clock = OsClock;

  template <typename T>
  using Atomic = std::atomic<T>;

  using AtomicFlag = std::atomic_flag;
};

}   // namespace rtos
