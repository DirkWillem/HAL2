#include "mock_system.h"

namespace doubles {

std::unique_ptr<MockCriticalSectionInterface>
    MockCriticalSectionInterface::inst{nullptr};

void MockCriticalSectionInterface::Enter() noexcept {
  instance().EnterCriticalSection();
}

void MockCriticalSectionInterface::Exit() noexcept {
  instance().ExitCriticalSection();
}

MockCriticalSectionInterface&
MockCriticalSectionInterface::instance() noexcept {
  if (inst == nullptr) {
    inst = std::make_unique<testing::NiceMock<MockCriticalSectionInterface>>();
  }

  return *inst;
}

void MockCriticalSectionInterface::Reset() noexcept {
  inst.reset();
}

}   // namespace doubles