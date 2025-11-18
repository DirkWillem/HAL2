module;

#include <memory>

#include <gmock/gmock.h>

export module hal2.testing.helpers:mocks.performance_timer;

import hal.abstract;

namespace hal2::testing::helpers {

export class MockPerformanceTimer {
 public:
  MOCK_METHOD(void, MockEnable, ());
  MOCK_METHOD(uint32_t, MockGet, ());

  static void Enable() { MockInstance().MockEnable(); }

  static uint32_t Get() { return MockInstance().MockGet(); }

  static MockPerformanceTimer& MockInstance() {
    auto& ip = inst_ptr();
    if (ip == nullptr) {
      ip = std::make_unique<::testing::NiceMock<MockPerformanceTimer>>();
    }

    return *ip;
  }

  static void Reset() { inst_ptr().reset(); }

 private:
  static std::unique_ptr<MockPerformanceTimer>& inst_ptr() {
    static std::unique_ptr<MockPerformanceTimer> instance{nullptr};
    return instance;
  }
};

static_assert(hal::PerformanceTimer<MockPerformanceTimer>);

}   // namespace hal2::testing::helpers