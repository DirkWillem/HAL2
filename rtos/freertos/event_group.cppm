module;

#include <optional>

#include <FreeRTOS.h>
#include <event_groups.h>

export module rtos.freertos:event_group;

import hstd;

import :time;

namespace rtos {

// There are 8 bits in an event group if 16 bit ticks are used, otherwise 24
#ifdef configUSE_16_BIT_TICKS
inline constexpr auto NumEventBits = 8;
#else
inline constexpr auto NumEventBits = 24;
#endif

export class EventGroup {
 public:
  static constexpr auto NumBits = NumEventBits;

  EventGroup()
      : handle{xEventGroupCreateStatic(&buffer)} {}

  uint32_t SetBits(uint32_t bits) noexcept {
    return xEventGroupSetBits(handle, bits);
  }

  void SetBitsFromInterrupt(uint32_t bits) noexcept {
    BaseType_t higher_prio_task_woken;
    const auto result =
        xEventGroupSetBitsFromISR(handle, bits, &higher_prio_task_woken);

    if (result != pdFAIL) {
      portYIELD_FROM_ISR(higher_prio_task_woken);
    }
  }

  uint32_t ClearBits(uint32_t bits) noexcept {
    return xEventGroupClearBits(handle, bits);
  }

  void ClearBitsFromInterrupt(uint32_t bits) noexcept {
    const auto result = xEventGroupClearBitsFromISR(handle, bits);

    if (result == pdPASS) {
      portYIELD_FROM_ISR(pdTRUE);
    }
  }

  std::optional<uint32_t> Wait(uint32_t            bits_to_wait,
                               hstd::Duration auto timeout,
                               bool                clear_on_exit = true,
                               bool wait_for_all = false) noexcept {
    const auto result = xEventGroupWaitBits(
        handle, bits_to_wait, clear_on_exit ? pdTRUE : pdFALSE,
        wait_for_all ? pdTRUE : pdFALSE, ToTicks(timeout));

    const auto masked_result = result & bits_to_wait;

    if (wait_for_all) {
      if (masked_result == bits_to_wait) {
        return {bits_to_wait};
      }

      return {};
    }

    if (masked_result != 0) {
      return {masked_result};
    }

    return {};
  }

 private:
  StaticEventGroup_t buffer{};
  EventGroupHandle_t handle;
};

}   // namespace rtos