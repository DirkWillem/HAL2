#pragma once

import hal.abstract;
import hal.stm32h5;

import rtos.check;

extern "C" {

void HAL_IncTick();

[[maybe_unused]] void SysTick_Handler() {
  HAL_IncTick();

  if constexpr (rtos::IsRtosUsed<rtos::FreeRtosMarker>()) {
    extern void xPortSysTickHandler(void);
    xPortSysTickHandler();
  }
}

[[maybe_unused, noreturn]] void HardFault_Handler() {
  while (true) {}
}

// RTOS-Related interrupts
void PendSV_Handler() {
  if constexpr (rtos::IsRtosUsed<rtos::FreeRtosMarker>()) {
    extern void xPortPendSVHandler(void);
    xPortPendSVHandler();
  }
}

void SVC_Handler() {
  if constexpr (rtos::IsRtosUsed<rtos::FreeRtosMarker>()) {
    extern void vPortSVCHandler(void);
    vPortSVCHandler();
  }
}
}