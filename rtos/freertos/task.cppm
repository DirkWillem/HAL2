module;

#include <array>
#include <optional>

#include <FreeRTOS.h>
#include <task.h>

export module rtos.freertos:task;

import hstd;

import :time;

namespace rtos {

export inline constexpr auto MinStackSize =
    configMINIMAL_STACK_SIZE * sizeof(StackType_t);

export inline constexpr auto MiniStackSize       = MinStackSize;
export inline constexpr auto SmallStackSize      = 128 * sizeof(StackType_t);
export inline constexpr auto MediumStackSize     = 256 * sizeof(StackType_t);
export inline constexpr auto LargeStackSize      = 512 * sizeof(StackType_t);
export inline constexpr auto ExtraLargeStackSize = 1024 * sizeof(StackType_t);
export inline constexpr auto HugeStackSize       = 2048 * sizeof(StackType_t);

export template <std::size_t SS>
struct StackSizeMarker {};

export inline constexpr StackSizeMarker<MiniStackSize>  MiniStackSizeMarker{};
export inline constexpr StackSizeMarker<SmallStackSize> SmallStackSizeMarker{};
export inline constexpr StackSizeMarker<MediumStackSize>
                                                        MediumStackSizeMarker{};
export inline constexpr StackSizeMarker<LargeStackSize> LargeStackSizeMarker{};
export inline constexpr StackSizeMarker<ExtraLargeStackSize>
    ExtraLargeStackSizeMarker{};

/**
 * Reference to a FreeRTOS task
 */
class TaskRef {
 public:
  explicit TaskRef(TaskHandle_t handle) noexcept
      : handle{handle} {}

  void NotifySetBitsFromInterrupt(uint32_t value) noexcept {
    BaseType_t higher_prio_task_woken{pdFALSE};

    xTaskNotifyFromISR(handle, value, eSetBits, &higher_prio_task_woken);
    portYIELD_FROM_ISR(higher_prio_task_woken);
  }

  void NotifySetBits(uint32_t value) noexcept {
    xTaskNotify(handle, value, eSetBits);
  }

 private:
  TaskHandle_t handle{};
};

/**
 * Base for a FreeRTOS Task
 * @tparam T Task implementation type
 * @tparam StackSize Task stack size
 */
export template <typename T, std::size_t StackSize = MinStackSize>
class Task {
  static_assert(StackSize % sizeof(StackType_t) == 0,
                "Stack size must be a multiple of the word size");

  static constexpr auto StackSizeWords = StackSize / sizeof(StackType_t);
  static_assert(StackSizeWords >= configMINIMAL_STACK_SIZE,
                "Stack size must be at least configMINIMAL_STACK_SIZE");

 public:
  TaskRef& ref() & noexcept { return task_ref; }

  /**
   * Returns whether a stop of the task was requested
   * @return Whether a stop of the task was requested
   *
   * @note For FreeRTOS tasks, stops are never requested
   */
  [[nodiscard]] constexpr bool StopRequested() const noexcept { return false; }

 protected:
  /**
   * Constructor
   * @param name Name of the task
   * @param prio Priority of the task
   */
  explicit Task(const char* name,
                UBaseType_t prio = configMAX_PRIORITIES - 1) noexcept
      : task_ref{xTaskCreateStatic(&Task::StaticInvoke, name, StackSizeWords,
                                   this, prio, stack.data(), &handle)} {}

  std::optional<uint32_t>
  WaitForNotification(hstd::Duration auto timeout,
                      uint32_t            bits_to_clear_on_entry = 0,
                      uint32_t            bits_to_clear_on_exit  = 0) noexcept {
    uint32_t   notification_value{};
    const auto result =
        xTaskNotifyWait(bits_to_clear_on_entry, bits_to_clear_on_exit,
                        &notification_value, ToTicks(timeout));

    if (result == pdPASS) {
      return {notification_value};
    } else {
      return {};
    }
  }

 private:
  static void StaticInvoke(void* data) noexcept {
    auto self = static_cast<T*>(data);
    (*self)();
  }

  StaticTask_t                            handle{};
  std::array<StackType_t, StackSizeWords> stack{};
  TaskRef                                 task_ref;
};

export template <typename T, std::size_t StackSize = MinStackSize>
class LambdaTask : public T {
  static_assert(StackSize % sizeof(StackType_t) == 0,
                "Stack size must be a multiple of the word size");

  static constexpr auto StackSizeWords = StackSize / sizeof(StackType_t);
  static_assert(StackSizeWords >= configMINIMAL_STACK_SIZE,
                "Stack size must be at least configMINIMAL_STACK_SIZE");

 public:
  LambdaTask(const char* name, T lambda,
             UBaseType_t prio = configMAX_PRIORITIES - 1)
      : T{lambda}
      , task_ref{xTaskCreateStatic(&LambdaTask::StaticInvoke, name,
                                   StackSizeWords, this, prio, stack.data(),
                                   &handle)} {}

  LambdaTask(const char*                                 name,
             [[maybe_unused]] StackSizeMarker<StackSize> stack_size_marker,
             T lambda, UBaseType_t prio = configMAX_PRIORITIES - 1) noexcept
      : T{lambda}
      , task_ref{xTaskCreateStatic(&LambdaTask::StaticInvoke, name,
                                   StackSizeWords, this, prio, stack.data(),
                                   &handle)} {}

 private:
  static void StaticInvoke(void* data) noexcept {
    auto self = static_cast<T*>(data);
    (*self)();
  }

  StaticTask_t handle{};
  alignas(StackType_t) std::array<StackType_t, StackSizeWords> stack{};
  TaskRef task_ref;
};

template <typename T>
LambdaTask(const char*, T, UBaseType_t) -> LambdaTask<T, MinStackSize>;

template <typename T, std::size_t StackSize>
LambdaTask(const char*, StackSizeMarker<StackSize>, T)
    -> LambdaTask<T, StackSize>;

template <typename T, std::size_t StackSize>
LambdaTask(const char*, StackSizeMarker<StackSize>, T, UBaseType_t)
    -> LambdaTask<T, StackSize>;

}   // namespace rtos