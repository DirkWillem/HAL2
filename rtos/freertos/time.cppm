module;

#include <chrono>

#include <FreeRTOS.h>

export module rtos.freertos:time;

import hstd;

namespace rtos {

export constexpr auto TicksToMs(TickType_t ticks) noexcept {
  return std::chrono::milliseconds{pdTICKS_TO_MS(ticks)};
}

export constexpr auto ToTicks(hstd::ToDuration auto duration) noexcept {
  return pdMS_TO_TICKS(std::chrono::duration_cast<std::chrono::milliseconds>(
                           hstd::MakeDuration(duration))
                           .count());
}

}   // namespace rtos