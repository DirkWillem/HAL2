#include "uart.h"

namespace stm32h7::detail {

constexpr void EnableUartClk(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: __HAL_RCC_USART1_CLK_ENABLE(); break;
  case UartId::Usart2: __HAL_RCC_USART2_CLK_ENABLE(); break;
  case UartId::Usart3: __HAL_RCC_USART3_CLK_ENABLE(); break;
  case UartId::Uart4: __HAL_RCC_UART4_CLK_ENABLE(); break;
  case UartId::Uart5: __HAL_RCC_UART5_CLK_ENABLE(); break;
  case UartId::Usart6: __HAL_RCC_USART6_CLK_ENABLE(); break;
  case UartId::Uart7: __HAL_RCC_UART7_CLK_ENABLE(); break;
  case UartId::Uart8: __HAL_RCC_UART8_CLK_ENABLE(); break;
  case UartId::LpUart1: __HAL_RCC_LPUART1_CLK_ENABLE(); break;
  }
}

[[nodiscard]] constexpr IRQn_Type GetIrqn(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: return USART1_IRQn;
  case UartId::Usart2: return USART2_IRQn;
  case UartId::Usart3: return USART3_IRQn;
  case UartId::Uart4: return UART4_IRQn;
  case UartId::Uart5: return UART5_IRQn;
  case UartId::Usart6: return USART6_IRQn;
  case UartId::Uart7: return UART7_IRQn;
  case UartId::Uart8: return UART8_IRQn;
  case UartId::LpUart1: return LPUART1_IRQn;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t
ToHalStopBits(hal::UartStopBits stop_bits) noexcept {
  switch (stop_bits) {
  case hal::UartStopBits::Half: return UART_STOPBITS_0_5;
  case hal::UartStopBits::One: return UART_STOPBITS_1;
  case hal::UartStopBits::OneAndHalf: return UART_STOPBITS_1_5;
  case hal::UartStopBits::Two: return UART_STOPBITS_2;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t ToHalParity(hal::UartParity parity) {
  switch (parity) {
  case hal::UartParity::Even: return USART_PARITY_EVEN;
  case hal::UartParity::Odd: return USART_PARITY_ODD;
  case hal::UartParity::None: return USART_PARITY_NONE;
  }

  std::unreachable();
}

void SetupUartNoFc(UartId id, UART_HandleTypeDef& huart, unsigned baud,
                   hal::UartParity   parity,
                   hal::UartStopBits stop_bits) noexcept {
  // Enable UART clock
  EnableUartClk(id);

  // Set up handle
  huart.Instance = GetUartPointer(id);
  huart.Init     = {
          .BaudRate   = baud,
          .WordLength = USART_WORDLENGTH_8B,
          .StopBits   = ToHalStopBits(stop_bits),
          .Parity     = ToHalParity(parity),
          .Mode       = USART_MODE_TX_RX,
          .HwFlowCtl  = UART_HWCONTROL_NONE,
  };
  HAL_UART_Init(&huart);
}

void InitializeUartForPollMode(UART_HandleTypeDef& huart) noexcept {
  HAL_UARTEx_SetTxFifoThreshold(&huart, UART_TXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_SetRxFifoThreshold(&huart, UART_RXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_DisableFifoMode(&huart);
}

void InitializeUartForInterruptMode(UartId              id,
                                    UART_HandleTypeDef& huart) noexcept {
  const auto irqn = GetIrqn(id);
  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);
}

}   // namespace stm32h7::detail