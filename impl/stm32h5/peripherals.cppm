module;

#include <chrono>
#include <utility>

#include <stm32h5xx_hal.h>

export module hal.stm32h5:peripherals;

import hstd;

namespace stm32h5 {

export enum class UartId {
  Usart1,
  Usart2,
  Usart3,
  Uart4,
  Uart5,
  Usart6,
  LpUart1,
};

export [[nodiscard]] constexpr USART_TypeDef*
GetUartPointer(UartId uart) noexcept {
  switch (uart) {
  case UartId::Usart1: return USART1;
  case UartId::Usart2: return USART2;
  case UartId::Usart3: return USART3;
  case UartId::Uart4: return UART4;
  case UartId::Uart5: return UART5;
  case UartId::Usart6: return USART6;
  case UartId::LpUart1: return LPUART1;
  }

  std::unreachable();
}

export [[nodiscard]] consteval UartId
UartIdFromName(std::string_view name) noexcept {
  return hstd::StaticMap<std::string_view, UartId, 7>(
      name, {{
                {"USART1", UartId::Usart1},
                {"USART2", UartId::Usart2},
                {"USART3", UartId::Usart3},
                {"UART4", UartId::Uart4},
                {"UART5", UartId::Uart5},
                {"USART6", UartId::Usart6},
                {"LPUART1", UartId::LpUart1},
            }});
}

export enum SpiId {
  Spi1,
  Spi2,
  Spi3,
  Spi4,
};

export [[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: return SPI1;
  case SpiId::Spi2: return SPI2;
  case SpiId::Spi3: return SPI3;
  case SpiId::Spi4: return SPI4;
  }

  std::unreachable();
}

}   // namespace stm32h5