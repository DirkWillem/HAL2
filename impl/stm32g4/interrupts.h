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

/**
 * I2C Interrupts
 */
#define I2C_EV_IRQ_HANDLER(Name)                                  \
  void Name##_EV_IRQHandler() {                                   \
    constexpr auto Inst = stm32g4::I2cIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32g4::I2c<Inst>>()) { \
      stm32g4::I2c<Inst>::instance().HandleEventInterrupt();      \
    }                                                             \
  }

I2C_EV_IRQ_HANDLER(I2C1)
I2C_EV_IRQ_HANDLER(I2C2)
I2C_EV_IRQ_HANDLER(I2C3)
I2C_EV_IRQ_HANDLER(I2C4)

#define I2C_ER_IRQ_HANDLER(Name)                                  \
  void Name##_ER_IRQHandler() {                                   \
    constexpr auto Inst = stm32g4::I2cIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32g4::I2c<Inst>>()) { \
      stm32g4::I2c<Inst>::instance().HandleErrorInterrupt();      \
    }                                                             \
  }

I2C_ER_IRQ_HANDLER(I2C1)
I2C_ER_IRQ_HANDLER(I2C2)
I2C_ER_IRQ_HANDLER(I2C3)
I2C_ER_IRQ_HANDLER(I2C4)

#define HANDLE_I2C_ERROR_CALLBACK(Inst)                    \
  if constexpr (hal::IsPeripheralInUse<stm32g4::Inst>()) { \
    if (hi2c == &stm32g4::Inst::instance().hi2c) {        \
      stm32g4::Inst::instance().Error();                   \
    }                                                      \
  }

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {
  HANDLE_I2C_ERROR_CALLBACK(I2c1)
  HANDLE_I2C_ERROR_CALLBACK(I2c2)
  HANDLE_I2C_ERROR_CALLBACK(I2c3)
  HANDLE_I2C_ERROR_CALLBACK(I2c4)
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
}
