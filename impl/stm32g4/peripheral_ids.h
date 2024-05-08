#pragma once

#include <string_view>
#include <utility>

#include <stm32g4xx_hal.h>

namespace stm32g4 {

enum class UartId {
  Usart1,
  Usart2,
  Usart3,
  Uart4,
  Uart5,
  LpUart1,
};

[[nodiscard]] constexpr USART_TypeDef* GetUartPointer(UartId uart) noexcept {
  switch (uart) {
  case UartId::Usart1: return USART1;
  case UartId::Usart2: return USART2;
  case UartId::Usart3: return USART3;
  case UartId::Uart4: return UART4;
  case UartId::Uart5: return UART5;
  case UartId::LpUart1: return LPUART1;
  }

  std::unreachable();
}

[[nodiscard]] consteval UartId UartIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "USART1"sv) {
    return UartId::Usart1;
  } else if (name == "USART2"sv) {
    return UartId::Usart2;
  } else if (name == "USART3"sv) {
    return UartId::Usart3;
  } else if (name == "UART4"sv) {
    return UartId::Uart4;
  } else if (name == "UART5"sv) {
    return UartId::Uart5;
  } else if (name == "LPUART1"sv) {
    return UartId::LpUart1;
  }

  std::unreachable();
}

}   // namespace stm32g4