#pragma once


namespace stm32g4 {

class CriticalSectionInterface {
 public:
  static void Enter() noexcept;
  static void Exit() noexcept;
};

}