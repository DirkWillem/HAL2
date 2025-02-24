#include <stm32g0xx_hal.h>

#include <hal/peripheral.h>

#include <stm32g0/tim.h>
#include <stm32g0/dma.h>
#include <stm32g0/i2s.h>
#include <stm32g0/pin_interrupt.h>
#include <stm32g0/uart.h>

template <unsigned P>
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

    if constexpr (PinInt::PinInterruptActive(P, hal::Edge::Falling)) {
      if (__HAL_GPIO_EXTI_GET_FALLING_IT(GetHalPin(P))) {
        __HAL_GPIO_EXTI_CLEAR_FALLING_IT(GetHalPin(P));
        PinInt::instance().HandleInterrupt<P, hal::Edge::Falling>();
      }
    }
  }
}

// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile

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

#define HANDLE_UART_TX_CALLBACK(Inst)                      \
  if constexpr (hal::IsPeripheralInUse<stm32g0::Inst>()) { \
    if (huart == &stm32g0::Inst::instance().huart) {       \
      stm32g0::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  HANDLE_UART_TX_CALLBACK(Usart1)
  HANDLE_UART_TX_CALLBACK(Usart2)
  HANDLE_UART_TX_CALLBACK(Usart3)
  HANDLE_UART_TX_CALLBACK(Usart4)
  HANDLE_UART_TX_CALLBACK(Usart5)
  HANDLE_UART_TX_CALLBACK(Usart6)
  HANDLE_UART_TX_CALLBACK(LpUart1)
  HANDLE_UART_TX_CALLBACK(LpUart2)
}

#define HANDLE_SPI_RECEIVE_CALLBACK(Inst)                  \
  if constexpr (hal::IsPeripheralInUse<stm32g0::Inst>()) { \
    if (hspi == &stm32g0::Inst::instance().hspi) {         \
      stm32g0::Inst::instance().ReceiveComplete();         \
    }                                                      \
  }

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi) {
  HANDLE_SPI_RECEIVE_CALLBACK(Spi1);
  HANDLE_SPI_RECEIVE_CALLBACK(Spi2);
  HANDLE_SPI_RECEIVE_CALLBACK(Spi3);
}

[[maybe_unused]] void TIM1_BRK_UP_TRG_COM_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim1)
}

[[maybe_unused]] void TIM1_CC_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim1)
}

[[maybe_unused]] void TIM2_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim2)
}

[[maybe_unused]] void TIM3_TIM4_IRQHandle() {
  HANDLE_PERIPHERAL_IRQ(Tim3)
  HANDLE_PERIPHERAL_IRQ(Tim4)
}

[[maybe_unused]] void TIM6_DAC_LPTIM1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim6)
}

[[maybe_unused]] void TIM7_LPTIM2_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim7)
}

[[maybe_unused]] void TIM14_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim14)
}

[[maybe_unused]] void TIM15_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim15)
}

[[maybe_unused]] void TIM16_FDCAN_IT0_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim16)
}

[[maybe_unused]] void TIM17_FDCAN_IT1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Tim17)
}

#define HANDLE_TIM_PERIOD_ELAPSED_CB(Inst)                 \
  if constexpr (hal::IsPeripheralInUse<stm32g0::Inst>()) { \
    if (htim == &stm32g0::Inst::instance().htim) {         \
      stm32g0::Inst::instance().PeriodElapsed();           \
    }                                                      \
  }

[[maybe_unused]] void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim1)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim2)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim3)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim4)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim6)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim7)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim14)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim15)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim16)
  HANDLE_TIM_PERIOD_ELAPSED_CB(Tim17)
}

[[maybe_unused]] void EXTI0_1_IRQHandler() {
  HandlePinInterrupt<0>();
  HandlePinInterrupt<1>();
}

[[maybe_unused]] void EXTI2_3_IRQHandler() {
  HandlePinInterrupt<2>();
  HandlePinInterrupt<3>();
}

[[maybe_unused]] void EXTI4_15_IRQHandler() {
  HandlePinInterrupt<4>();
  HandlePinInterrupt<5>();
  HandlePinInterrupt<6>();
  HandlePinInterrupt<7>();
  HandlePinInterrupt<8>();
  HandlePinInterrupt<9>();
  HandlePinInterrupt<10>();
  HandlePinInterrupt<11>();
  HandlePinInterrupt<12>();
  HandlePinInterrupt<13>();
  HandlePinInterrupt<14>();
  HandlePinInterrupt<15>();
}
}