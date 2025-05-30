module;

#include <FreeRTOS.h>

#include <task.h>

export module rtos.freertos;

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

export [[noreturn]] inline void StartScheduler() noexcept {
  vTaskStartScheduler();
  while (true) {}
}

}   // namespace rtos