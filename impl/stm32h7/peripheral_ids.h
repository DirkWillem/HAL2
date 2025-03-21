#pragma once

#include <string_view>
#include <utility>

#include <constexpr_tools/static_mapping.h>

#include <stm32h7xx.h>

namespace stm32h7 {

enum class UartId {
  Usart1,
  Usart2,
  Usart3,
  Uart4,
  Uart5,
  Usart6,
  Uart7,
  Uart8,
  LpUart1,
};

[[nodiscard]] constexpr USART_TypeDef* GetUartPointer(UartId uart) noexcept {
  switch (uart) {
  case UartId::Usart1: return USART1;
  case UartId::Usart2: return USART2;
  case UartId::Usart3: return USART3;
  case UartId::Uart4: return UART4;
  case UartId::Uart5: return UART5;
  case UartId::Usart6: return USART6;
  case UartId::Uart7: return UART7;
  case UartId::Uart8: return UART8;
  case UartId::LpUart1: return LPUART1;
  }

  std::unreachable();
}

[[nodiscard]] consteval UartId UartIdFromName(std::string_view name) noexcept {
  return ct::StaticMap<std::string_view, UartId, 9>(
      name, {{
                {"USART1", UartId::Usart1},
                {"USART2", UartId::Usart2},
                {"USART3", UartId::Usart3},
                {"UART4", UartId::Uart4},
                {"UART5", UartId::Uart5},
                {"USART6", UartId::Usart6},
                {"UART7", UartId::Uart7},
                {"UART8", UartId::Uart8},
                {"LPUART1", UartId::LpUart1},
            }});
}

enum class SpiId {
  Spi1,
  Spi2,
  Spi3,
  Spi4,
  Spi5,
  Spi6,
};

[[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId spi) noexcept {
  switch (spi) {
  case SpiId::Spi1: return SPI1;
  case SpiId::Spi2: return SPI2;
  case SpiId::Spi3: return SPI3;
  case SpiId::Spi4: return SPI4;
  case SpiId::Spi5: return SPI5;
  case SpiId::Spi6: return SPI6;
  }

  std::unreachable();
}

[[nodiscard]] consteval SpiId SpiIdFromName(std::string_view name) noexcept {
  return ct::StaticMap<std::string_view, SpiId, 6>(name,
                                                   {{
                                                       {"SPI1", SpiId::Spi1},
                                                       {"SPI2", SpiId::Spi2},
                                                       {"SPI3", SpiId::Spi3},
                                                       {"SPI4", SpiId::Spi4},
                                                       {"SPI5", SpiId::Spi5},
                                                       {"SPI6", SpiId::Spi6},
                                                   }});
}

enum class I2sId {
  I2s1,
  I2s2,
  I2s3,
};

[[nodiscard]] consteval I2sId I2sIdFromName(std::string_view name) noexcept {
  return ct::StaticMap<std::string_view, I2sId, 3>(name,
                                                   {{
                                                       {"I2S1", I2sId::I2s1},
                                                       {"I2S2", I2sId::I2s2},
                                                       {"I2S3", I2sId::I2s3},
                                                   }});
}

}   // namespace stm32h7