#include "mock_clock.h"

#include <exception>

namespace hal::test::doubles {

std::unique_ptr<MockClock> MockClock::inst{nullptr};

void MockClock::Reset() noexcept {
  inst.reset();
}

MockClock& MockClock::instance() {
   if (inst == nullptr) {
     inst = std::make_unique<MockClock>();
   }

  return *inst;
}

MockClock::DurationType MockClock::GetTimeSinceBoot() noexcept {
  return instance().MockGetTimeSinceBoot();
}

void MockClock::BlockFor(DurationType dt) noexcept {
  instance().MockBlockFor(dt);
}




}   // namespace hal::test::doubles
