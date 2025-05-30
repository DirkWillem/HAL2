module;

#include <array>
#include <concepts>

#include <FreeRTOS.h>
#include <task.h>

export module rtos.freertos:task;

import hstd;

namespace rtos {

inline constexpr auto MinStackSize = configMINIMAL_STACK_SIZE;

/**
 * Base for a FreeRTOS Task
 * @tparam T Task implementation type
 * @tparam StackSize Task stack size
 */
export template <typename T, std::size_t StackSize = MinStackSize>
class Task {
 protected:
  /**
   * Constructor
   * @param name Name of the task
   * @param prio Priority of the task
   */
  explicit Task(const char* name,
                UBaseType_t prio = configMAX_PRIORITIES - 1) noexcept {
    xTaskCreateStatic(&Task::StaticInvoke, name, StackSize, this, prio,
                      stack.data(), &handle);
  }

 private:
  static void StaticInvoke(void* data) noexcept {
    auto self = static_cast<T*>(data);
    (*self)();
  }

  StaticTask_t                       handle{};
  std::array<StackType_t, StackSize> stack{};
};

export template <typename T, std::size_t StackSize = MinStackSize>
class LambdaTask : public T {
 public:
  LambdaTask(const char* name, T lambda,
             UBaseType_t prio = configMAX_PRIORITIES - 1)
      : T{lambda} {
    xTaskCreateStatic(&LambdaTask::StaticInvoke, name, StackSize, this, prio,
                      stack.data(), &handle);
  }

 private:
  static void StaticInvoke(void* data) noexcept {
    auto self = static_cast<T*>(data);
    (*self)();
  }
  StaticTask_t                       handle{};
  std::array<StackType_t, StackSize> stack{};
};

template <typename T>
LambdaTask(const char*, T, UBaseType_t) -> LambdaTask<T, MinStackSize>;

}   // namespace rtos