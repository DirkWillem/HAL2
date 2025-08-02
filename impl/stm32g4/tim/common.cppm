module;

#include <stm32g4xx_hal.h>

export module hal.stm32g4:tim.common;

import hal.abstract;

import :dma;
import :peripherals;

namespace stm32g4 {

namespace concepts {
export template <typename Impl>
concept ChannelPeriodElapsedCallback = requires(Impl impl) {
  { impl.ChannelPeriodElapsedCallback() };
};

}   // namespace concepts

export template <TimId Id, hal::DmaPriority Prio = hal::DmaPriority::Low,
                 typename Cb = hstd::Empty>
using TimUpdateDma = DmaChannel<Id, TimDmaRequest::Update, Prio, Cb>;

export void EnableTimClock(TimId tim) {
  switch (tim) {
  case TimId::Tim1: __HAL_RCC_TIM1_CLK_ENABLE(); break;
  case TimId::Tim2: __HAL_RCC_TIM2_CLK_ENABLE(); break;
  case TimId::Tim3: __HAL_RCC_TIM3_CLK_ENABLE(); break;
  case TimId::Tim4: __HAL_RCC_TIM4_CLK_ENABLE(); break;
  case TimId::Tim5: __HAL_RCC_TIM5_CLK_ENABLE(); break;
  case TimId::Tim6: __HAL_RCC_TIM6_CLK_ENABLE(); break;
  case TimId::Tim7: __HAL_RCC_TIM7_CLK_ENABLE(); break;
  case TimId::Tim8: __HAL_RCC_TIM8_CLK_ENABLE(); break;
  case TimId::Tim15: __HAL_RCC_TIM15_CLK_ENABLE(); break;
  case TimId::Tim16: __HAL_RCC_TIM16_CLK_ENABLE(); break;
  case TimId::Tim17: __HAL_RCC_TIM17_CLK_ENABLE(); break;
  case TimId::Tim20: __HAL_RCC_TIM20_CLK_ENABLE(); break;
  }
}

export template <typename Impl>
void EnableTimInterrupt(TimId id) noexcept {
  switch (id) {
  case TimId::Tim1:
    EnableInterrupt<TIM1_BRK_TIM15_IRQn, Impl>();
    EnableInterrupt<TIM1_UP_TIM16_IRQn, Impl>();
    EnableInterrupt<TIM1_TRG_COM_TIM17_IRQn, Impl>();
    EnableInterrupt<TIM1_CC_IRQn, Impl>();
    break;
  case TimId::Tim2: EnableInterrupt<TIM2_IRQn, Impl>(); break;
  case TimId::Tim3: EnableInterrupt<TIM3_IRQn, Impl>(); break;
  case TimId::Tim4: EnableInterrupt<TIM4_IRQn, Impl>(); break;
  case TimId::Tim5: EnableInterrupt<TIM5_IRQn, Impl>(); break;
  case TimId::Tim6: EnableInterrupt<TIM6_DAC_IRQn, Impl>(); break;
  case TimId::Tim7: EnableInterrupt<TIM7_DAC_IRQn, Impl>(); break;
  case TimId::Tim8:
    EnableInterrupt<TIM8_BRK_IRQn, Impl>();
    EnableInterrupt<TIM8_UP_IRQn, Impl>();
    EnableInterrupt<TIM8_TRG_COM_IRQn, Impl>();
    EnableInterrupt<TIM8_CC_IRQn, Impl>();
    break;
  case TimId::Tim15: EnableInterrupt<TIM1_BRK_TIM15_IRQn, Impl>(); break;
  case TimId::Tim16: EnableInterrupt<TIM1_UP_TIM16_IRQn, Impl>(); break;
  case TimId::Tim17: EnableInterrupt<TIM1_TRG_COM_TIM17_IRQn, Impl>(); break;
  case TimId::Tim20:
    EnableInterrupt<TIM20_BRK_IRQn, Impl>();
    EnableInterrupt<TIM20_UP_IRQn, Impl>();
    EnableInterrupt<TIM20_TRG_COM_IRQn, Impl>();
    EnableInterrupt<TIM20_CC_IRQn, Impl>();
    break;
  }
}

export void DisableTimInterrupt(TimId id) noexcept {
  switch (id) {
  case TimId::Tim1:
    NVIC_DisableIRQ(TIM1_BRK_TIM15_IRQn);
    NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn);
    NVIC_DisableIRQ(TIM1_TRG_COM_TIM17_IRQn);
    NVIC_DisableIRQ(TIM1_CC_IRQn);
    break;
  case TimId::Tim2: NVIC_DisableIRQ(TIM2_IRQn); break;
  case TimId::Tim3: NVIC_DisableIRQ(TIM3_IRQn); break;
  case TimId::Tim4: NVIC_DisableIRQ(TIM4_IRQn); break;
  case TimId::Tim5: NVIC_DisableIRQ(TIM5_IRQn); break;
  case TimId::Tim6: NVIC_DisableIRQ(TIM6_DAC_IRQn); break;
  case TimId::Tim7: NVIC_DisableIRQ(TIM7_DAC_IRQn); break;
  case TimId::Tim8:
    NVIC_DisableIRQ(TIM8_BRK_IRQn);
    NVIC_DisableIRQ(TIM8_UP_IRQn);
    NVIC_DisableIRQ(TIM8_TRG_COM_IRQn);
    NVIC_DisableIRQ(TIM8_CC_IRQn);
    break;
  case TimId::Tim15: NVIC_DisableIRQ(TIM1_BRK_TIM15_IRQn); break;
  case TimId::Tim16: NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn); break;
  case TimId::Tim20:
    NVIC_DisableIRQ(TIM20_BRK_IRQn);
    NVIC_DisableIRQ(TIM20_UP_IRQn);
    NVIC_DisableIRQ(TIM20_TRG_COM_IRQn);
    NVIC_DisableIRQ(TIM20_CC_IRQn);
    break;
  }
}

}   // namespace stm32g4