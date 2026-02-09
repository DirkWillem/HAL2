module;

#include <chrono>

#include <gmock/gmock.h>

export module hal2.testing.helpers:mocks.clock;

import hstd;

namespace hal2::testing::helpers {

/**
 * @brief Provides a mock clock implementation that conforms to the STL clock
 * requirements.
 */
export class MockClock {
 public:
  // std::chrono clock implementation
  using rep        = uint32_t;
  using period     = std::milli;
  using duration   = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<MockClock, duration>;

  static constexpr auto is_steady = false;

  [[nodiscard]] static time_point now() { return MockInstance().GetNow(); }

  // Helper methods.

  [[nodiscard]] static time_point TimePointFromSeconds(float s) {
    return time_point{std::chrono::milliseconds{
        static_cast<uint32_t>(std::round(s * 1000.F))}};
  }

  [[nodiscard]] static time_point
  TimePointFromDuration(hstd::Duration auto duration) {
    return time_point{hstd::MakeDuration(duration)};
  }

  // Mocked methods.
  MOCK_METHOD(time_point, GetNow, ());

  // Singleton constructor & reset.
  static MockClock& MockInstance() {
    auto& ip = inst_ptr();
    if (ip == nullptr) {
      ip = std::make_unique<::testing::NiceMock<MockClock>>();
    }

    return *ip;
  }

  static void Reset() { inst_ptr().reset(); }

 private:
  static std::unique_ptr<MockClock>& inst_ptr() {
    static std::unique_ptr<MockClock> instance{nullptr};
    return instance;
  }
};

}   // namespace hal2::testing::helpers