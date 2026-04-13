#pragma once

import hal.abstract;
import hal.stm32h5;

import rtos.check;

template <unsigned P>
void HandlePinInterrupt() noexcept {
  using namespace stm32h5;
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

#define DMA_IRQ_HANDLER(Inst, Chan)                                                     \
  void GPDMA##Inst##_Channel##Chan##_IRQHandler() {                                     \
    if constexpr (hal::IsPeripheralInUse<stm32h5::Dma<stm32h5::DmaImplMarker>>()) {     \
      if (stm32h5::Dma<stm32h5::DmaImplMarker>::ChannelInUse<Inst, Chan>()) {           \
        stm32h5::Dma<stm32h5::DmaImplMarker>::instance().HandleInterrupt<Inst, Chan>(); \
      }                                                                                 \
    }                                                                                   \
  }

void GPDMA1_Channel0_IRQHandler() {
  if constexpr (hal::IsPeripheralInUse<stm32h5::Dma<stm32h5::DmaImplMarker>>()) {
    if (stm32h5::Dma<stm32h5::DmaImplMarker>::ChannelInUse<1, 0>()) {
      stm32h5::Dma<stm32h5::DmaImplMarker>::instance().HandleInterrupt<1, 0>();
    }
  }
}
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

#define HANDLE_UART_RECEIVE_CALLBACK(Inst)                                       \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) {                       \
    if (huart == &stm32h5::Inst::instance().huart) {                             \
      stm32h5::Inst::instance().ReceiveComplete(static_cast<std::size_t>(size)); \
    }                                                                            \
  }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size){
    HANDLE_UART_RECEIVE_CALLBACK(Usart1) HANDLE_UART_RECEIVE_CALLBACK(Usart2)
        HANDLE_UART_RECEIVE_CALLBACK(Usart3) HANDLE_UART_RECEIVE_CALLBACK(Uart4)
            HANDLE_UART_RECEIVE_CALLBACK(Uart5) HANDLE_UART_RECEIVE_CALLBACK(LpUart1)}
#define HANDLE_UART_TX_CALLBACK(Inst)                      \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (huart == &stm32h5::Inst::instance().huart) {       \
      stm32h5::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart){
    HANDLE_UART_TX_CALLBACK(Usart1) HANDLE_UART_TX_CALLBACK(Usart2) HANDLE_UART_TX_CALLBACK(Usart3)
        HANDLE_UART_TX_CALLBACK(Uart4) HANDLE_UART_TX_CALLBACK(Uart5)
            HANDLE_UART_TX_CALLBACK(LpUart1)}

/**
 * SPI Interrupts
 */

#define SPI_IRQ_HANDLER(Name)                                     \
  void Name##_IRQHandler() {                                      \
    constexpr auto Inst = stm32h5::SpiIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32h5::Spi<Inst>>()) { \
      stm32h5::Spi<Inst>::instance().HandleInterrupt();           \
    }                                                             \
  }

#define SPI_I2S_IRQ_HANDLER(SpiName, I2sName)                        \
  void SpiName##_IRQHandler() {                                      \
    constexpr auto SpiInst = stm32h5::SpiIdFromName(#SpiName);       \
    if constexpr (hal::IsPeripheralInUse<stm32h5::Spi<SpiInst>>()) { \
      stm32h5::Spi<SpiInst>::instance().HandleInterrupt();           \
    }                                                                \
    constexpr auto I2sInst = stm32h5::I2sIdFromName(#I2sName);       \
    if constexpr (hal::IsPeripheralInUse<stm32h5::I2s<I2sInst>>()) { \
      stm32h5::I2s<I2sInst>::instance().HandleInterrupt();           \
    }                                                                \
  }

SPI_I2S_IRQ_HANDLER(SPI1, I2S1) SPI_I2S_IRQ_HANDLER(SPI2, I2S2) void SPI3_IRQHandler() {
  constexpr auto SpiInst = stm32h5::SpiIdFromName("SPI3");
  if constexpr (hal::IsPeripheralInUse<stm32h5::Spi<SpiInst>>()) {
    stm32h5::Spi<SpiInst>::instance().HandleInterrupt();
  }
  constexpr auto I2sInst = stm32h5::I2sIdFromName("I2S3");
  if constexpr (hal::IsPeripheralInUse<stm32h5::I2s<I2sInst>>()) {
    stm32h5::I2s<I2sInst>::instance().HandleInterrupt();
  }
}
SPI_IRQ_HANDLER(SPI4)

#define HANDLE_SPI_RX_CALLBACK(Inst)                       \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hspi == &stm32h5::Inst::instance().hspi) {         \
      stm32h5::Inst::instance().ReceiveComplete();         \
    }                                                      \
  }

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi){
    HANDLE_SPI_RX_CALLBACK(Spi1) HANDLE_SPI_RX_CALLBACK(Spi2) HANDLE_SPI_RX_CALLBACK(Spi3)
        HANDLE_SPI_RX_CALLBACK(Spi4)}
#define HANDLE_SPI_TX_CALLBACK(Inst)                       \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hspi == &stm32h5::Inst::instance().hspi) {         \
      stm32h5::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi){
    HANDLE_SPI_TX_CALLBACK(Spi1) HANDLE_SPI_TX_CALLBACK(Spi2) HANDLE_SPI_TX_CALLBACK(Spi3)
        HANDLE_SPI_TX_CALLBACK(Spi4)}

#define HANDLE_I2S_HALF_RECEIVE_CALLBACK(Inst)             \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hspi == &stm32h5::Inst::instance().hi2s) {         \
      stm32h5::Inst::instance().HalfReceiveComplete();     \
    }                                                      \
  }

#define HANDLE_I2S_RECEIVE_CALLBACK(Inst)                  \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hspi == &stm32h5::Inst::instance().hi2s) {         \
      stm32h5::Inst::instance().ReceiveComplete();         \
    }                                                      \
  }

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef* hspi) {
  HANDLE_I2S_HALF_RECEIVE_CALLBACK(I2s1)
  HANDLE_I2S_HALF_RECEIVE_CALLBACK(I2s2)
  HANDLE_I2S_HALF_RECEIVE_CALLBACK(I2s3)
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef* hspi){HANDLE_I2S_RECEIVE_CALLBACK(I2s1)
                                                         HANDLE_I2S_RECEIVE_CALLBACK(I2s2)
                                                             HANDLE_I2S_RECEIVE_CALLBACK(I2s3)}

/**
 * I2C Interrupts
 */

#define I2C_EV_IRQ_HANDLER(Name)                                  \
  void Name##_EV_IRQHandler() {                                   \
    constexpr auto Inst = stm32h5::I2cIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32h5::I2c<Inst>>()) { \
      stm32h5::I2c<Inst>::instance().HandleEventInterrupt();      \
    }                                                             \
  }

I2C_EV_IRQ_HANDLER(I2C1);
I2C_EV_IRQ_HANDLER(I2C2);
I2C_EV_IRQ_HANDLER(I2C3);

#define I2C_ER_IRQ_HANDLER(Name)                                  \
  void Name##_ER_IRQHandler() {                                   \
    constexpr auto Inst = stm32h5::I2cIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32h5::I2c<Inst>>()) { \
      stm32h5::I2c<Inst>::instance().HandleErrorInterrupt();      \
    }                                                             \
  }

I2C_ER_IRQ_HANDLER(I2C1);
I2C_ER_IRQ_HANDLER(I2C2);
I2C_ER_IRQ_HANDLER(I2C3);

#define HANDLE_I2C_ERROR_CALLBACK(Inst)                    \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hi2c == &stm32h5::Inst::instance().hi2c) {         \
      stm32h5::Inst::instance().Error();                   \
    }                                                      \
  }

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {
  HANDLE_I2C_ERROR_CALLBACK(I2c1);
  HANDLE_I2C_ERROR_CALLBACK(I2c2);
  HANDLE_I2C_ERROR_CALLBACK(I2c3);
}
  
#define HANDLE_I2C_RX_CALLBACK(Inst)                       \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hi2c == &stm32h5::Inst::instance().hi2c) {         \
      stm32h5::Inst::instance().RxComplete();              \
    }                                                      \
  }

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {
  HANDLE_I2C_RX_CALLBACK(I2c1);
  HANDLE_I2C_RX_CALLBACK(I2c2);
  HANDLE_I2C_RX_CALLBACK(I2c3);
}

#define HANDLE_I2C_TX_CALLBACK(Inst)                       \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hi2c == &stm32h5::Inst::instance().hi2c) {         \
      stm32h5::Inst::instance().TxComplete();              \
    }                                                      \
  }

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {
  HANDLE_I2C_TX_CALLBACK(I2c1);
  HANDLE_I2C_TX_CALLBACK(I2c2);
  HANDLE_I2C_TX_CALLBACK(I2c3);
}

#define HANDLE_I2C_MEM_RX_CALLBACK(Inst)                   \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hi2c == &stm32h5::Inst::instance().hi2c) {         \
      stm32h5::Inst::instance().MemRxComplete();           \
    }                                                      \
  }

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
  HANDLE_I2C_MEM_RX_CALLBACK(I2c1);
  HANDLE_I2C_MEM_RX_CALLBACK(I2c2);
  HANDLE_I2C_MEM_RX_CALLBACK(I2c3);
}

#define HANDLE_I2C_MEM_TX_CALLBACK(Inst)                   \
  if constexpr (hal::IsPeripheralInUse<stm32h5::Inst>()) { \
    if (hi2c == &stm32h5::Inst::instance().hi2c) {         \
      stm32h5::Inst::instance().MemTxComplete();           \
    }                                                      \
  }

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef* hi2c) {
  HANDLE_I2C_MEM_TX_CALLBACK(I2c1);
  HANDLE_I2C_MEM_TX_CALLBACK(I2c2);
  HANDLE_I2C_MEM_TX_CALLBACK(I2c3);
}

//
// Pin interrupts
//

#define EXTI_HANDLER(N)                          \
  [[maybe_unused]] void EXTI##N##_IRQHandler() { \
    HandlePinInterrupt<N>();                     \
  }

EXTI_HANDLER(0);
EXTI_HANDLER(1);
EXTI_HANDLER(2);
EXTI_HANDLER(3);
EXTI_HANDLER(4);
EXTI_HANDLER(5);
EXTI_HANDLER(6);
EXTI_HANDLER(7);
EXTI_HANDLER(8);
EXTI_HANDLER(9);
EXTI_HANDLER(10);
EXTI_HANDLER(11);
EXTI_HANDLER(12);
EXTI_HANDLER(13);
EXTI_HANDLER(14);
EXTI_HANDLER(15)
}