#pragma once

import hal.abstract;
import hal.stm32g4;

import rtos.check;

/*template <unsigned P>
void HandlePinInterrupt() noexcept {
  using namespace stm32g0;
  using PinInt = PinInterrupt<PinInterruptImplMarker>;

  if constexpr (hal::IsPeripheralInUse<PinInt>()) {
    if constexpr (PinInt::PinInterruptActive(P, hal::Edge::Rising)) {
      if (__HAL_GPIO_EXTI_GET_RISING_IT(GetHalPin(P))) {
        __HAL_GPIO_EXTI_CLEAR_RISING_IT(GetHalPin(P));
        PinInt::instance().HandleInterrupt<P, hal::Edge::Rising>();
      }
    }

    if constexplpr (PinInt::PinInterruptActive(P, hal::Edge::Falling)) {
      if (__HAL_GPIO_EXTI_GET_FALLING_IT(GetHalPin(P))) {
        __HAL_GPIO_EXTI_CLEAR_FALLING_IT(GetHalPin(P));
        PinInt::instance().HandleInterrupt<P, hal::Edge::Falling>();
      }
    }
  }
}*/

// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile

extern "C" {

[[maybe_unused]] void SysTick_Handler() {
  HAL_IncTick();

  if constexpr (rtos::IsRtosUsed<rtos::FreeRtos>()) {
    extern void xPortSysTickHandler(void);
    xPortSysTickHandler();
  }
}

[[maybe_unused, noreturn]] void HardFault_Handler() {
  while (true) {}
}

// RTOS-Related interrupts
void PendSV_Handler() {
  if constexpr (rtos::IsRtosUsed<rtos::FreeRtos>()) {
    extern void xPortPendSVHandler(void);
    xPortPendSVHandler();
  }
}

void SVC_Handler() {
  if constexpr (rtos::IsRtosUsed<rtos::FreeRtos>()) {
    extern void vPortSVCHandler(void);
    vPortSVCHandler();
  }
}

/**
 * DMA Interrupts
 */
static_assert(hal::Peripheral<stm32g4::Dma<stm32g4::DmaImplMarker>>);

#define DMA_IRQ_HANDLER(Inst, Chan)                                           \
  void DMA##Inst##_Channel##Chan##_IRQHandler() {                             \
    if constexpr (hal::IsPeripheralInUse<                                     \
                      stm32g4::Dma<stm32g4::DmaImplMarker>>()) {              \
      if (stm32g4::Dma<stm32g4::DmaImplMarker>::ChannelInUse<Inst, Chan>()) { \
        stm32g4::Dma<stm32g4::DmaImplMarker>::instance()                      \
            .HandleInterrupt<Inst, Chan>();                                   \
      }                                                                       \
    }                                                                         \
  }

DMA_IRQ_HANDLER(1, 1)
DMA_IRQ_HANDLER(1, 2)
DMA_IRQ_HANDLER(1, 3)
DMA_IRQ_HANDLER(1, 4)
DMA_IRQ_HANDLER(1, 5)
DMA_IRQ_HANDLER(1, 6)
DMA_IRQ_HANDLER(1, 7)
DMA_IRQ_HANDLER(1, 8)

DMA_IRQ_HANDLER(2, 1)
DMA_IRQ_HANDLER(2, 2)
DMA_IRQ_HANDLER(2, 3)
DMA_IRQ_HANDLER(2, 4)
DMA_IRQ_HANDLER(2, 5)
DMA_IRQ_HANDLER(2, 6)
DMA_IRQ_HANDLER(2, 7)
DMA_IRQ_HANDLER(2, 8)

/**
 * UART Interrupts
 */
#define UART_IRQ_HANDLER(Name)                                     \
  void Name##_IRQHandler() {                                       \
    constexpr auto Inst = stm32g4::UartIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32g4::Uart<Inst>>()) { \
      stm32g4::Uart<Inst>::instance().HandleInterrupt();           \
    }                                                              \
  }

UART_IRQ_HANDLER(USART1)
UART_IRQ_HANDLER(USART2)
UART_IRQ_HANDLER(USART3)
UART_IRQ_HANDLER(UART4)
UART_IRQ_HANDLER(UART5)
UART_IRQ_HANDLER(LPUART1)

#define HANDLE_UART_RECEIVE_CALLBACK(Inst)                 \
  if constexpr (hal::IsPeripheralInUse<stm32g4::Inst>()) { \
    if (huart == &stm32g4::Inst::instance().huart) {       \
      stm32g4::Inst::instance().ReceiveComplete(           \
          static_cast<std::size_t>(size));                 \
    }                                                      \
  }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size) {
  HANDLE_UART_RECEIVE_CALLBACK(Usart1)
  HANDLE_UART_RECEIVE_CALLBACK(Usart2)
  HANDLE_UART_RECEIVE_CALLBACK(Usart3)
  HANDLE_UART_RECEIVE_CALLBACK(Uart4)
  HANDLE_UART_RECEIVE_CALLBACK(Uart5)
  HANDLE_UART_RECEIVE_CALLBACK(LpUart1)
}

#define HANDLE_UART_TX_CALLBACK(Inst)                      \
  if constexpr (hal::IsPeripheralInUse<stm32g4::Inst>()) { \
    if (huart == &stm32g4::Inst::instance().huart) {       \
      stm32g4::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  HANDLE_UART_TX_CALLBACK(Usart1) HANDLE_UART_TX_CALLBACK(Usart2)
      HANDLE_UART_TX_CALLBACK(Usart3) HANDLE_UART_TX_CALLBACK(Uart4)
          HANDLE_UART_TX_CALLBACK(Uart5) HANDLE_UART_TX_CALLBACK(LpUart1)
}

;
// }
//
// [[maybe_unused]] void TIM1_BRK_UP_TRG_COM_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim1)
// }
//
// [[maybe_unused]] void TIM1_CC_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim1)
// }
//
// [[maybe_unused]] void TIM2_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim2)
// }
//
// [[maybe_unused]] void TIM3_TIM4_IRQHandle() {
//   HANDLE_PERIPHERAL_IRQ(Tim3)
//   HANDLE_PERIPHERAL_IRQ(Tim4)
// }
//
// [[maybe_unused]] void TIM6_DAC_LPTIM1_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim6)
// }
//
// [[maybe_unused]] void TIM7_LPTIM2_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim7)
// }
//
// [[maybe_unused]] void TIM14_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim14)
// }
//
// [[maybe_unused]] void TIM15_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim15)
// }
//
// [[maybe_unused]] void TIM16_FDCAN_IT0_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim16)
// }
//
// [[maybe_unused]] void TIM17_FDCAN_IT1_IRQHandler() {
//   HANDLE_PERIPHERAL_IRQ(Tim17)
// }
//
// #define HANDLE_TIM_PERIOD_ELAPSED_CB(Inst)                 \
//   if constexpr (hal::IsPeripheralInUse<stm32g0::Inst>()) { \
//     if (htim == &stm32g0::Inst::instance().htim) {         \
//       stm32g0::Inst::instance().PeriodElapsed();           \
//     }                                                      \
//   }
//
// [[maybe_unused]] void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
// {
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim1)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim2)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim3)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim4)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim6)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim7)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim14)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim15)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim16)
//   HANDLE_TIM_PERIOD_ELAPSED_CB(Tim17)
// }
//
// [[maybe_unused]] void EXTI0_1_IRQHandler() {
//   HandlePinInterrupt<0>();
//   HandlePinInterrupt<1>();
// }
//
// [[maybe_unused]] void EXTI2_3_IRQHandler() {
//   HandlePinInterrupt<2>();
//   HandlePinInterrupt<3>();
// }
//
// [[maybe_unused]] void EXTI4_15_IRQHandler() {
//   HandlePinInterrupt<4>();
//   HandlePinInterrupt<5>();
//   HandlePinInterrupt<6>();
//   HandlePinInterrupt<7>();
//   HandlePinInterrupt<8>();
//   HandlePinInterrupt<9>();
//   HandlePinInterrupt<10>();
//   HandlePinInterrupt<11>();
//   HandlePinInterrupt<12>();
//   HandlePinInterrupt<13>();
//   HandlePinInterrupt<14>();
//   HandlePinInterrupt<15>();
// }
}