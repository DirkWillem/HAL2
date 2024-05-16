#pragma once

#include <string_view>
#include <utility>

#include <stm32g0xx_hal.h>

namespace stm32g0 {

enum class UartId {
  Usart1,
  Usart2,
  Usart3,
  Usart4,
  Usart5,
  Usart6,
  LpUart1,
  LpUart2,
};

[[nodiscard]] constexpr USART_TypeDef* GetUartPointer(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: return USART1;
  case UartId::Usart2: return USART2;
  case UartId::Usart3: return USART3;
  case UartId::Usart4: return USART4;
  case UartId::Usart5: return USART5;
  case UartId::Usart6: return USART6;
  case UartId::LpUart1: return LPUART1;
  case UartId::LpUart2: return LPUART2;
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
  } else if (name == "USART4"sv) {
    return UartId::Usart4;
  } else if (name == "USART5"sv) {
    return UartId::Usart5;
  } else if (name == "USART6"sv) {
    return UartId::Usart6;
  } else if (name == "LPUART1"sv) {
    return UartId::LpUart1;
  } else if (name == "LPUART2"sv) {
    return UartId::LpUart2;
  }

  std::unreachable();
}

enum class SpiId {
  Spi1,
  Spi2,
  Spi3,
};

[[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: return SPI1;
  case SpiId::Spi2: return SPI2;
  case SpiId::Spi3: return SPI3;
  }

  std::unreachable();
}

[[nodiscard]] consteval SpiId SpiIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "SPI1"sv) {
    return SpiId::Spi1;
  } else if (name == "SPI2"sv) {
    return SpiId::Spi2;
  } else if (name == "SPI3"sv) {
    return SpiId::Spi3;
  }

  std::unreachable();
}

enum class I2sId {
  I2s1,
  I2s2,
};

[[nodiscard]] constexpr SpiId GetSpiForI2s(I2sId id) noexcept {
  switch (id) {
  case I2sId::I2s1: return SpiId::Spi1;
  case I2sId::I2s2: return SpiId::Spi2;
  }

  std::unreachable();
}

[[nodiscard]] constexpr SPI_TypeDef* GetI2sPointer(I2sId id) noexcept {
  return GetSpiPointer(GetSpiForI2s(id));
}

[[nodiscard]] consteval I2sId I2sIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "I2S1"sv) {
    return I2sId::I2s1;
  } else if (name == "I2S2"sv) {
    return I2sId::I2s2;
  }

  std::unreachable();
}

}   // namespace stm32g0