#pragma once

#include <chrono>
#include <memory>

#include <gmock/gmock.h>

namespace hal::test::doubles {

class MockClock {
 public:
  using DurationType = std::chrono::milliseconds;

  MOCK_METHOD(DurationType, MockGetTimeSinceBoot, (), ());
  MOCK_METHOD(void, MockBlockFor, (DurationType block_for), ());

  static void       Reset() noexcept;
  static MockClock& instance();

  static DurationType GetTimeSinceBoot() noexcept;
  static void         BlockFor(DurationType dt) noexcept;

 private:
  static std::unique_ptr<MockClock> inst;
};

}   // namespace hal::test::doubles