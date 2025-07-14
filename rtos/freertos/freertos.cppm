module;

#include <array>

#include <FreeRTOS.h>

#include <task.h>

export module rtos.freertos;

export import :event_group;
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
};

}   // namespace rtos