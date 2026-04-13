module;

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>

#include <gmock/gmock.h>

export module hal.test.helpers:mocks.rtos_i2c;

import hal.abstract;

namespace hal::test::helpers {

export class MockRtosI2c {
 public:
  MOCK_METHOD(I2cReadResult, ReadMemory,
              (uint16_t device_address, uint16_t memory_address, std::span<std::byte> dest,
               std::chrono::milliseconds timeout, std::optional<std::size_t> size),
              ());

  I2cReadResult ReadMemory(uint16_t device_address, uint16_t memory_address,
                           std::span<std::byte> dest, std::chrono::milliseconds timeout) {
    return ReadMemory(device_address, memory_address, dest, timeout, {});
  }
};

}   // namespace hal::test::helpers