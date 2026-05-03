module;

#include <chrono>
#include <optional>

#include <gmock/gmock.h>

export module rtos.freertos.test_helpers;

import hstd;

namespace rtos::test {

/**
 * @brief Mock for the FreeRTOS Event Group.
 */
export class MockEventGroup {
  MOCK_METHOD(uint32_t, SetBits, (uint32_t bits));
  MOCK_METHOD(uint32_t, SetBitsFromInterrupt, (uint32_t bits));
  MOCK_METHOD(uint32_t, ClearBits, (uint32_t bits));
  MOCK_METHOD(uint32_t, ClearBitsFromInterrupt, (uint32_t bits));
  MOCK_METHOD(uint32_t, ReadBits, ());

  MOCK_METHOD(std::optional<uint32_t>, MockWait,
              (uint32_t bits_to_wait, std::chrono::milliseconds timeout_ms, bool clear_on_exit,
               bool wait_for_all));

  std::optional<uint32_t> Wait(uint32_t bits_to_wait, hstd::Duration auto timeout,
                               bool clear_on_exit = true, bool wait_for_all = false) {
    return MockWait(bits_to_wait, std::chrono::duration_cast<std::chrono::milliseconds>(timeout),
                    clear_on_exit, wait_for_all);
  }
};

}   // namespace rtos::test