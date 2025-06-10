module;

#include <array>
#include <concepts>
#include <optional>

#include <FreeRTOS.h>
#include <queue.h>

export module rtos.freertos:queue;

import hstd;

import :time;

namespace rtos {

export template <std::copyable T, std::size_t QS>
class Queue {
 public:
  using Item                      = T;
  static constexpr auto QueueSize = QS;

  Queue() noexcept
      : queue{xQueueCreateStatic(
            QueueSize, sizeof(T),
            hstd::ReinterpretSpanMut<uint8_t>(buffer).data(), &static_queue)} {}

  bool Enqueue(const T& item, hstd::Duration auto timeout) noexcept {
    return xQueueSendToBack(queue, &item, ToTicks(timeout)) == pdPASS;
  }

  bool TryEnqueue(const T& item) noexcept {
    return xQueueSendToBack(queue, &item, 0) == pdPASS;
  }

  void EnqueueFromInterrupt(const T& item) noexcept {
    BaseType_t higher_prio_task_woken;
    const auto result =
        xQueueSendToBackFromISR(queue, &item, &higher_prio_task_woken);

    if (result == pdPASS) {
      portYIELD_FROM_ISR(higher_prio_task_woken);
    }
  }

  std::optional<T> Dequeue(hstd::Duration auto timeout) noexcept {
    T result;
    if (xQueueReceive(queue, &result, ToTicks(timeout)) == pdPASS) {
      return {result};
    }

    return {};
  }

 private:
  std::array<T, QueueSize> buffer{};

  StaticQueue_t static_queue{};
  QueueHandle_t queue;
};

}   // namespace rtos