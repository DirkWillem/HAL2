#include "uart.h"

namespace stm32g0::detail {

constexpr void EnableUartClk(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: __HAL_RCC_USART1_CLK_ENABLE(); break;
  case UartId::Usart2: __HAL_RCC_USART2_CLK_ENABLE(); break;
  case UartId::Usart3: __HAL_RCC_USART3_CLK_ENABLE(); break;
  case UartId::Usart4: __HAL_RCC_USART4_CLK_ENABLE(); break;
  case UartId::Usart5: __HAL_RCC_USART5_CLK_ENABLE(); break;
  case UartId::Usart6: __HAL_RCC_USART6_CLK_ENABLE(); break;
  case UartId::LpUart1: __HAL_RCC_LPUART1_CLK_ENABLE(); break;
  case UartId::LpUart2: __HAL_RCC_LPUART2_CLK_ENABLE(); break;
  }
}

[[nodiscard]] constexpr IRQn_Type GetIrqn(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: return USART1_IRQn;
  case UartId::Usart2: [[fallthrough]];
  case UartId::LpUart2: return USART2_LPUART2_IRQn;
  case UartId::Usart3: [[fallthrough]];
  case UartId::Usart4: [[fallthrough]];
  case UartId::Usart5: [[fallthrough]];
  case UartId::Usart6: [[fallthrough]];
  case UartId::LpUart1: return USART3_4_5_6_LPUART1_IRQn;
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

}   // namespace stm32g0::detail