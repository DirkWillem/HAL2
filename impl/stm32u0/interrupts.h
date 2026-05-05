#pragma once


extern "C" {

extern void HAL_IncTick();

[[maybe_unused]] void SysTick_Handler() {
  HAL_IncTick();
}

[[maybe_unused, noreturn]] void HardFault_Handler() {
  while (true) {}
}

}