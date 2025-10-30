#pragma once

import hal.abstract;
import hal.stm32h5;

import rtos.check;

extern "C" {

//
// System-related interrupts
//

extern void HAL_IncTick();

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

/**
 * @note When using FreeRTOS, the following interrupts are defined FreeRTOS:
 *  - \c PendSV_Handler
 *  - \c SVC_Handler
 */

//
// DMA interrupts
//

#define DMA_IRQ_HANDLER(Inst, Chan)                                           \
  void GPDMA##Inst##_Channel##Chan##_IRQHandler() {                           \
    if constexpr (hal::IsPeripheralInUse<                                     \
                      stm32h5::Dma<stm32h5::DmaImplMarker>>()) {              \
      if (stm32h5::Dma<stm32h5::DmaImplMarker>::ChannelInUse<Inst, Chan>()) { \
        stm32h5::Dma<stm32h5::DmaImplMarker>::instance()                      \
            .HandleInterrupt<Inst, Chan>();                                   \
      }                                                                       \
    }                                                                         \
  }

DMA_IRQ_HANDLER(1, 0)
DMA_IRQ_HANDLER(1, 1)
DMA_IRQ_HANDLER(1, 2)
DMA_IRQ_HANDLER(1, 3)
DMA_IRQ_HANDLER(1, 4)
DMA_IRQ_HANDLER(1, 5)
DMA_IRQ_HANDLER(1, 6)
DMA_IRQ_HANDLER(1, 7)

DMA_IRQ_HANDLER(2, 0)
DMA_IRQ_HANDLER(2, 1)
DMA_IRQ_HANDLER(2, 2)
DMA_IRQ_HANDLER(2, 3)
DMA_IRQ_HANDLER(2, 4)
DMA_IRQ_HANDLER(2, 5)
DMA_IRQ_HANDLER(2, 6)
DMA_IRQ_HANDLER(2, 7)

//
// UART interrupts
//

#define UART_IRQ_HANDLER(Name)                                     \
  void Name##_IRQHandler() {                                       \
    constexpr auto Inst = stm32h5::UartIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32h5::Uart<Inst>>()) { \
      stm32h5::Uart<Inst>::instance().HandleInterrupt();           \
    }                                                              \
  }

UART_IRQ_HANDLER(USART1)
UART_IRQ_HANDLER(USART2)
UART_IRQ_HANDLER(USART3)
UART_IRQ_HANDLER(UART4)
UART_IRQ_HANDLER(UART5)
UART_IRQ_HANDLER(USART6)
UART_IRQ_HANDLER(LPUART1)

#define HANDLE_UART_RECEIVE_CALLBACK(Inst)                 \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (huart == &stm32h5::Inst::instance().huart) {       \
      stm32h5::Inst::instance().ReceiveComplete(           \
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
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (huart == &stm32h5::Inst::instance().huart) {       \
      stm32h5::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  HANDLE_UART_TX_CALLBACK(Usart1)
  HANDLE_UART_TX_CALLBACK(Usart2)
  HANDLE_UART_TX_CALLBACK(Usart3)
  HANDLE_UART_TX_CALLBACK(Uart4)
  HANDLE_UART_TX_CALLBACK(Uart5)
  HANDLE_UART_TX_CALLBACK(LpUart1)
}
}