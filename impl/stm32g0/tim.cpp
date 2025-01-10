#include "tim.h"

namespace stm32g0::detail {

void EnableTimClock(TimId tim) {
  switch (tim) {
  case TimId::Tim1:
    __HAL_RCC_TIM1_CLK_ENABLE();
    break;
    ;
  case TimId::Tim2:
    __HAL_RCC_TIM2_CLK_ENABLE();
    break;
    ;
  case TimId::Tim3:
    __HAL_RCC_TIM3_CLK_ENABLE();
    break;
    ;
  case TimId::Tim4:
    __HAL_RCC_TIM4_CLK_ENABLE();
    break;
    ;
  case TimId::Tim6:
    __HAL_RCC_TIM6_CLK_ENABLE();
    break;
    ;
  case TimId::Tim7:
    __HAL_RCC_TIM7_CLK_ENABLE();
    break;
    ;
  case TimId::Tim14:
    __HAL_RCC_TIM14_CLK_ENABLE();
    break;
    ;
  case TimId::Tim15:
    __HAL_RCC_TIM15_CLK_ENABLE();
    break;
    ;
  case TimId::Tim16:
    __HAL_RCC_TIM16_CLK_ENABLE();
    break;
    ;
  case TimId::Tim17:
    __HAL_RCC_TIM17_CLK_ENABLE();
    break;
    ;
  }
}

void EnableTimInterrupt(TimId id, uint32_t prio) {
  switch (id) {
  case TimId::Tim1:
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, prio);
    NVIC_SetPriority(TIM1_CC_IRQn, prio);
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    NVIC_EnableIRQ(TIM1_CC_IRQn);
    break;
  case TimId::Tim2:
    NVIC_SetPriority(TIM2_IRQn, prio);
    NVIC_EnableIRQ(TIM2_IRQn);
    break;
  case TimId::Tim3: [[fallthrough]];
  case TimId::Tim4:
    NVIC_SetPriority(TIM3_TIM4_IRQn, prio);
    NVIC_EnableIRQ(TIM3_TIM4_IRQn);
    break;
  case TimId::Tim6:
    NVIC_SetPriority(TIM6_DAC_LPTIM1_IRQn, prio);
    NVIC_EnableIRQ(TIM6_DAC_LPTIM1_IRQn);
    break;
  case TimId::Tim7:
    NVIC_SetPriority(TIM7_LPTIM2_IRQn, prio);
    NVIC_EnableIRQ(TIM7_LPTIM2_IRQn);
    break;
  case TimId::Tim14:
    NVIC_SetPriority(TIM14_IRQn, prio);
    NVIC_EnableIRQ(TIM14_IRQn);
    break;
  case TimId::Tim15:
    NVIC_SetPriority(TIM15_IRQn, prio);
    NVIC_EnableIRQ(TIM15_IRQn);
    break;
  case TimId::Tim16:
    NVIC_SetPriority(TIM16_FDCAN_IT0_IRQn, prio);
    NVIC_EnableIRQ(TIM16_FDCAN_IT0_IRQn);
    break;
  case TimId::Tim17:
    NVIC_SetPriority(TIM17_FDCAN_IT1_IRQn, prio);
    NVIC_EnableIRQ(TIM17_FDCAN_IT1_IRQn);
    break;
  }
}

}   // namespace stm32g0::detail