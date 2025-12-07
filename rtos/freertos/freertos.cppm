module;

#include <array>

#include <FreeRTOS.h>

#include <task.h>

export module rtos.freertos;

import rtos.concepts;

export import :event_group;
export import :mutex;
export import :queue;
export import :system;
export import :task;
export import :time;

extern "C" {

void vPortSetupTimerInterrupt() {}

void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t pxTask,
                                   [[maybe_unused]] char*        pcTaskName) {
  while (true) {}
}
}

namespace rtos {

export struct SysCallIrqPrio {
  static uint32_t GetPriority(auto) noexcept {
    return configMAX_SYSCALL_INTERRUPT_PRIORITY;
  }
};

export [[noreturn]] inline void StartScheduler() noexcept {
  vTaskStartScheduler();
  while (true) {}
}

export struct FreeRtos {
  using TaskRef    = TaskRef;
  using EventGroup = EventGroup;
  using Mutex      = Mutex;
  using System     = System;

  static constexpr auto MiniStackSize       = rtos::MiniStackSize;
  static constexpr auto SmallStackSize      = rtos::SmallStackSize;
  static constexpr auto MediumStackSize     = rtos::MediumStackSize;
  static constexpr auto LargeStackSize      = rtos::LargeStackSize;
  static constexpr auto ExtraLargeStackSize = rtos::ExtraLargeStackSize;

  template <typename Impl, std::size_t StackSize = MediumStackSize>
  using Task = rtos::Task<Impl, StackSize>;
};

static_assert(rtos::concepts::Rtos<FreeRtos>);

}   // namespace rtos