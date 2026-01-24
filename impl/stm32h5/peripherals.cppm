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

export enum class SpiId { Spi1, Spi2, Spi3, Spi4 };

export [[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  using enum SpiId;

  switch (id) {
  case Spi1: return SPI1;
  case Spi2: return SPI2;
  case Spi3: return SPI3;
  case Spi4: return SPI4;
  }

  std::unreachable();
}

export [[nodiscard]] consteval SpiId
SpiIdFromName(std::string_view name) noexcept {
  using enum SpiId;

  if (name == "SPI1") {
    return Spi1;
  }

  if (name == "SPI2") {
    return Spi2;
  }

  if (name == "SPI3") {
    return Spi3;
  }

  if (name == "SPI4") {
    return Spi4;
  }

  std::unreachable();
}

export enum class I2sId { I2s1, I2s2, I2s3 };

export [[nodiscard]] consteval I2sId
I2sIdFromName(std::string_view name) noexcept {
  using enum I2sId;

  if (name == "I2S1") {
    return I2s1;
  }

  if (name == "I2S2") {
    return I2s2;
  }

  if (name == "I2S3") {
    return I2s3;
  }

  std::unreachable();
}

export [[nodiscard]] consteval SpiId GetSpiForI2s(I2sId id) {
  using enum SpiId;
  using enum I2sId;

  if (id == I2s1) {
    return Spi1;
  }
  if (id == I2s2) {
    return Spi2;
  }
  if (id == I2s3) {
    return Spi3;
  }
}

export enum class TimId {
  Tim1,
  Tim2,
  Tim3,
  Tim4,
  Tim5,
  Tim6,
  Tim7,
  Tim8,
  Tim12,
  Tim15
};

export [[nodiscard]] TIM_TypeDef* GetTimPointer(TimId id) {
  using enum TimId;

  if (id == Tim1) {
    return TIM1;
  }
  if (id == Tim2) {
    return TIM2;
  }
  if (id == Tim3) {
    return TIM3;
  }
  if (id == Tim4) {
    return TIM4;
  }
  if (id == Tim5) {
    return TIM5;
  }
  if (id == Tim6) {
    return TIM6;
  }
  if (id == Tim7) {
    return TIM7;
  }
  if (id == Tim8) {
    return TIM8;
  }
  if (id == Tim12) {
    return TIM12;
  }
  if (id == Tim15) {
    return TIM15;
  }

  std::unreachable();
}

}   // namespace stm32h5