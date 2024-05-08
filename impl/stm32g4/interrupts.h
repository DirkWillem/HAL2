#pragma once

#include <stm32g4xx_hal.h>

#include <stm32g4/dma.h>
#include <stm32g4/peripheral_ids.h>
#include <stm32g4/uart.h>

extern "C" {

void SysTick_Handler() {
  HAL_IncTick();
}

[[noreturn]] void HardFault_Handler() {
  while (true) {}
}

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
void LPUART1_IRQHandler() {
  constexpr auto Inst = stm32g4::UartIdFromName("LPUART1");
  if constexpr (hal::IsPeripheralInUse<stm32g4::Uart<Inst>>()) {
    stm32g4::Uart<Inst>::instance().HandleInterrupt();
  }
}

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

void DMA1_Channel1_IRQHandler() {
  static_assert(hal::IsPeripheralInUse<stm32g4::Dma<stm32g4::DmaImplMarker>>());
  static_assert(stm32g4::Dma<stm32g4::DmaImplMarker>::ChannelInUse<1, 1>());

  if constexpr (hal::IsPeripheralInUse<
                    stm32g4::Dma<stm32g4::DmaImplMarker>>()) {
    if constexpr (stm32g4::Dma<stm32g4::DmaImplMarker>::ChannelInUse<1, 1>()) {
      stm32g4::Dma<stm32g4::DmaImplMarker>::instance().HandleInterrupt<1, 1>();
    }
  }
}

void DMA1_Channel2_IRQHandler() {
  static_assert(hal::IsPeripheralInUse<stm32g4::Dma<stm32g4::DmaImplMarker>>());
  static_assert(stm32g4::Dma<stm32g4::DmaImplMarker>::ChannelInUse<1, 2>());

  if constexpr (hal::IsPeripheralInUse<
                    stm32g4::Dma<stm32g4::DmaImplMarker>>()) {
    if constexpr (stm32g4::Dma<stm32g4::DmaImplMarker>::ChannelInUse<1, 2>()) {
      stm32g4::Dma<stm32g4::DmaImplMarker>::instance().HandleInterrupt<1, 2>();
    }
  }
}
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
}
