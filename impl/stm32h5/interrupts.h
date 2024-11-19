#pragma once

#include <stm32h5xx_hal.h>

extern "C" {

void SysTick_Handler() {
  HAL_IncTick();
}

[[noreturn]] void HardFault_Handler() {
  const volatile auto* scb = SCB;
  while (true) {}
}
}