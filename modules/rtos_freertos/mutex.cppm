module;

#include <array>
#include <concepts>
#include <optional>

#include <FreeRTOS.h>
#include <semphr.h>

export module rtos.freertos:mutex;

import hstd;

import :time;

namespace rtos {

/**
 * @brief Mutex implementation.
 */
export class Mutex {
 public:
  Mutex() noexcept
      : semaphore{}
      , handle{xSemaphoreCreateMutexStatic(&semaphore)} {}

  Mutex(const Mutex&)            = delete;
  Mutex(Mutex&&)                 = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&)      = delete;
  ~Mutex() noexcept              = default;

  /**
   * @brief Attempts to lock the mutex.
   *
   * @param timeout Timeout to wait for locking the mutex.
   * @return Whether locking the mutex was successful.
   */
  bool Lock(hstd::Duration auto timeout) noexcept {
    return xSemaphoreTake(handle, ToTicks(timeout)) == pdTRUE;
  }

  /**
   * @brief Unlocks the mutex.
   */
  void Unlock() noexcept { xSemaphoreGive(handle); }

  /**
   * @brief Attempts to lock the mutex. If locking succeeds, performs the
   * provided action.
   *
   * @param timeout Timeout to wait for locking the mutex.
   * @param action Action to perform.
   * @return \c true if the mutex was successfully locked and the action
   * performed, \c false otherwise.
   */
  bool WithLocked(hstd::Duration auto     timeout,
                  std::invocable<> auto&& action) noexcept {
    if (Lock(timeout)) {
      action();
      Unlock();
      return true;
    }

    return false;
  }

 private:
  StaticSemaphore_t semaphore;
  SemaphoreHandle_t handle;
};

}   // namespace rtos