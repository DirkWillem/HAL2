// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile

#pragma once

#include <internal/peripheral_availability.h>

import hal.abstract;
import hal.stm32u0;

extern "C" {
extern void HAL_IncTick();

[[maybe_unused]] void SysTick_Handler() {
  HAL_IncTick();

  stm32u0::InterruptHook<SysTick_IRQn>::operator()();
}

[[maybe_unused, noreturn]] void HardFault_Handler() {
  while (true) {}
}

// DMA interrupts.

/**
 * Handles a single DMA instance / channel combination in a DMA interrupt handler.
 * @param Inst DMA instance.
 * @param Chan DMA channel.
 */
#define HANDLE_DMA_IRQ(Inst, Chan)                                                    \
  if constexpr (hal::IsPeripheralInUse<stm32u0::Dma<stm32u0::DmaImplMarker>>()) {     \
    if (stm32u0::Dma<stm32u0::DmaImplMarker>::ChannelInUse<Inst, Chan>()) {           \
      stm32u0::Dma<stm32u0::DmaImplMarker>::instance().HandleInterrupt<Inst, Chan>(); \
    }                                                                                 \
  }

/** @brief Interrupt handler for the \c DMA1_Channel1 interrupt. */
[[maybe_unused]] void DMA1_Channel1_IRQHandler() {
  HANDLE_DMA_IRQ(1, 1);
}

/** @brief Interrupt handler for the \c DMA1_Channel2_3 interrupt. */
[[maybe_unused]] void DMA1_Channel2_3_IRQHandler() {
  HANDLE_DMA_IRQ(1, 2);
  HANDLE_DMA_IRQ(1, 3);
}

/** @brief Interrupt handler for the \c DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR interrupt. */
#if (N_DMA1_CHANS == 7) && (N_DMA2_CHANS == 5)
[[maybe_unused]] void DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX_OVR_IRQHandler() {
  HANDLE_DMA_IRQ(1, 4);
  HANDLE_DMA_IRQ(1, 5);
  HANDLE_DMA_IRQ(1, 6);
  HANDLE_DMA_IRQ(1, 7);

  HANDLE_DMA_IRQ(2, 1);
  HANDLE_DMA_IRQ(2, 2);
  HANDLE_DMA_IRQ(2, 3);
  HANDLE_DMA_IRQ(2, 4);
  HANDLE_DMA_IRQ(2, 5);
}
#endif

/**
 * Handles the interrupt of a single peripheral.
 * @param Periph Peripheral to handle the interrupt for.
 */
#define HANDLE_PERIPHERAL_IRQ(Periph)                        \
  if constexpr (hal::IsPeripheralInUse<stm32u0::Periph>()) { \
    stm32u0::Periph::instance().HandleInterrupt();           \
  }

// UART interrupt handlers.

/** @brief Handler for the \c USART1 interrupt. */
[[maybe_unused]] void USART1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart1);
}

/** @brief Handler for the \c USART2_LPUART2 interrupt. */
[[maybe_unused]] void USART2_LPUART2_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart2);
  HANDLE_PERIPHERAL_IRQ(LpUart2);
}

/** @brief Handler for the \c USART3_LPUART1 interrupt. */
[[maybe_unused]] void USART3_LPUART1_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart3);
  HANDLE_PERIPHERAL_IRQ(LpUart1);
}

#ifdef HAS_LPUART_3
/** @brief Handler for the \c USART4_LPUART3 interrupt. */
[[maybe_unused]] void USART4_LPUART3_IRQHandler() {
  HANDLE_PERIPHERAL_IRQ(Usart4);
  HANDLE_PERIPHERAL_IRQ(LpUart3);
}
#endif

/**
 * Invokes the UART receive complete callback for a given UART instance.
 * @param Inst UART instance to invoke the callback for.
 */
#define HANDLE_UART_RX_CALLBACK(Inst)                                            \
  if constexpr (hal::IsPeripheralInUse<stm32u0::Inst>()) {                       \
    if (huart == &stm32u0::Inst::instance().huart) {                             \
      stm32u0::Inst::instance().ReceiveComplete(static_cast<std::size_t>(size)); \
    }                                                                            \
  }

/**
 * UART receive complete callback.
 * @param huart UART handle the callback was invoked for.
 * @param size Number of received bytes.
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size) {
#ifdef HAS_USART_1_2_3_4
  HANDLE_UART_RX_CALLBACK(Usart1);
  HANDLE_UART_RX_CALLBACK(Usart2);
  HANDLE_UART_RX_CALLBACK(Usart3);
  HANDLE_UART_RX_CALLBACK(Usart4);
#endif
#ifdef HAS_LPUART_1_2
  HANDLE_UART_RX_CALLBACK(LpUart1);
  HANDLE_UART_RX_CALLBACK(LpUart2);
#endif
#ifdef HAS_LPUART_3
  HANDLE_UART_RX_CALLBACK(LpUart3);
#endif
}

/**
 * Invokes the UART transmit complete callback for a given UART instance.
 * @param Inst UART instance to invoke the callback for.
 */
#define HANDLE_UART_TX_CALLBACK(Inst)                      \
  if constexpr (hal::IsPeripheralInUse<stm32u0::Inst>()) { \
    if (huart == &stm32u0::Inst::instance().huart) {       \
      stm32u0::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

/**
 * UART transmit complete callback.
 * @param huart UART handle the callback was invoked for.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
#ifdef HAS_USART_1_2_3_4
  HANDLE_UART_TX_CALLBACK(Usart1);
  HANDLE_UART_TX_CALLBACK(Usart2);
  HANDLE_UART_TX_CALLBACK(Usart3);
  HANDLE_UART_TX_CALLBACK(Usart4);
#endif
#ifdef HAS_LPUART_1_2
  HANDLE_UART_TX_CALLBACK(LpUart1);
  HANDLE_UART_TX_CALLBACK(LpUart2);
#endif
#ifdef HAS_LPUART_3
  HANDLE_UART_TX_CALLBACK(LpUart3);
#endif
}
}