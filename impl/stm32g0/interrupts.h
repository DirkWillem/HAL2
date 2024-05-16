#include <stm32g0xx_hal.h>

#include <hal/peripheral.h>

#include <stm32g0/dma.h>
#include <stm32g0/i2s.h>
#include <stm32g0/uart.h>

extern "C" {

[[maybe_unused]] void SysTick_Handler() {
  HAL_IncTick();
}

[[maybe_unused, noreturn]] void HardFault_Handler() {
  while (true) {}
}

#define HANDLE_DMA_IRQ(Inst, Chan)                                          \
  if constexpr (hal::IsPeripheralInUse<                                     \
                    stm32g0::Dma<stm32g0::DmaImplMarker>>()) {              \
    if (stm32g0::Dma<stm32g0::DmaImplMarker>::ChannelInUse<Inst, Chan>()) { \
      stm32g0::Dma<stm32g0::DmaImplMarker>::instance()                      \
          .HandleInterrupt<Inst, Chan>();                                   \
    }                                                                       \
  }

[[maybe_unused]] void DMA1_Channel1_IRQHandler() {
  HANDLE_DMA_IRQ(1, 1)
}

[[maybe_unused]] void DMA1_Channel2_3_IRQHandler() {
  HANDLE_DMA_IRQ(1, 2)
  HANDLE_DMA_IRQ(1, 3)
}

[[maybe_unused]] void DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler() {
  HANDLE_DMA_IRQ(1, 4)
  HANDLE_DMA_IRQ(1, 5)
  HANDLE_DMA_IRQ(1, 6)
  HANDLE_DMA_IRQ(1, 7)

  HANDLE_DMA_IRQ(2, 1)
  HANDLE_DMA_IRQ(2, 2)
  HANDLE_DMA_IRQ(2, 3)
  HANDLE_DMA_IRQ(2, 4)
  HANDLE_DMA_IRQ(2, 5)
}

#define HANDLE_PERIPHERAL_IRQ(Periph)                        \
  if constexpr (hal::IsPeripheralInUse<stm32g0::Periph>()) { \
    stm32g0::Periph::instance().HandleInterrupt();           \
  }

[[maybe_unused]] void USART1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart1)
}

[[maybe_unused]] void USART2_LPUART2_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart2)
  HANDLE_PERIPHERAL_IRQ(LpUart2)
}

[[maybe_unused]] void USART3_4_5_6_LPUART1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart3)
  HANDLE_PERIPHERAL_IRQ(Usart4)
  HANDLE_PERIPHERAL_IRQ(Usart5)
  HANDLE_PERIPHERAL_IRQ(Usart6)
  HANDLE_PERIPHERAL_IRQ(LpUart1)
}

[[maybe_unused]] void SPI1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(I2s1);
}

[[maybe_unused]] void SPI2_3_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(I2s2);
}

#define HANDLE_UART_RECEIVE_CALLBACK(Inst)                 \
  if constexpr (hal::IsPeripheralInUse<stm32g0::Inst>()) { \
    if (huart == &stm32g0::Inst::instance().huart) {       \
      stm32g0::Inst::instance().ReceiveComplete(           \
          static_cast<std::size_t>(size));                 \
    }                                                      \
  }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size) {
  HANDLE_UART_RECEIVE_CALLBACK(Usart1)
  HANDLE_UART_RECEIVE_CALLBACK(Usart2)
  HANDLE_UART_RECEIVE_CALLBACK(Usart3)
  HANDLE_UART_RECEIVE_CALLBACK(Usart4)
  HANDLE_UART_RECEIVE_CALLBACK(Usart5)
  HANDLE_UART_RECEIVE_CALLBACK(Usart6)
  HANDLE_UART_RECEIVE_CALLBACK(LpUart1)
  HANDLE_UART_RECEIVE_CALLBACK(LpUart2)
}
}