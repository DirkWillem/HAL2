#pragma once

#include <gmock/gmock.h>

#include <memory>

namespace doubles {

class MockCriticalSectionInterface {
 public:
  MOCK_METHOD(void, EnterCriticalSection, (), (noexcept));
  MOCK_METHOD(void, ExitCriticalSection, (), (noexcept));

  static void Enter() noexcept;
  static void Exit() noexcept;

  static MockCriticalSectionInterface& instance() noexcept;
  static void                          Reset() noexcept;

 private:
  static std::unique_ptr<MockCriticalSectionInterface> inst;
};

}   // namespace doubles