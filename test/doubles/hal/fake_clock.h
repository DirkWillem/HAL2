#pragma once

#include <chrono>

#include <hal/clocks.h>

namespace hal::test::doubles {

class FakeClock {
 public:
  using DurationType = std::chrono::milliseconds;

  [[nodiscard]] constexpr static DurationType TimeSinceBoot() {
    return DurationType{};
  }
};

static_assert(hal::Clock<FakeClock>);

}   // namespace hal::test::doubles