module;

#include <chrono>
#include <utility>

#include <stm32g4xx_hal.h>

export module hal.stm32g4:peripherals;

namespace stm32g4 {

export enum class UartId {
  Usart1,
  Usart2,
  Usart3,
  Uart4,
  Uart5,
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
  case UartId::LpUart1: return LPUART1;
  }

  std::unreachable();
}

export [[nodiscard]] consteval UartId
UartIdFromName(std::string_view name) noexcept {
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

export enum class I2cId {
  I2c1,
  I2c2,
  I2c3,
  I2c4,
};

export [[nodiscard]] constexpr I2C_TypeDef* GetI2cPointer(I2cId id) noexcept {
  switch (id) {
  case I2cId::I2c1: return I2C1;
  case I2cId::I2c2: return I2C2;
  case I2cId::I2c3: return I2C3;
  case I2cId::I2c4: return I2C4;
  }

  std::unreachable();
}

export [[nodiscard]] consteval I2cId
I2cIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "I2C1"sv) {
    return I2cId::I2c1;
  } else if (name == "I2C2"sv) {
    return I2cId::I2c2;
  } else if (name == "I2C3"sv) {
    return I2cId::I2c3;
  } else if (name == "I2C4"sv) {
    return I2cId::I2c4;
  }

  std::unreachable();
}

export enum class SpiId {
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

export [[nodiscard]] consteval SpiId
SpiIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "SPI1"sv) {
    return SpiId::Spi1;
  } else if (name == "SPI2"sv) {
    return SpiId::Spi2;
  } else if (name == "SPI3"sv) {
    return SpiId::Spi3;
  } else if (name == "SPI4"sv) {
    return SpiId::Spi4;
  }

  std::unreachable();
}

export enum class I2sId {
  I2s2,
  I2s3,
};

export [[nodiscard]] constexpr SpiId GetSpiForI2s(I2sId id) noexcept {
  switch (id) {
  case I2sId::I2s2: return SpiId::Spi2;
  case I2sId::I2s3: return SpiId::Spi3;
  }

  std::unreachable();
}

export [[nodiscard]] constexpr SPI_TypeDef* GetI2sPointer(I2sId id) noexcept {
  return GetSpiPointer(GetSpiForI2s(id));
}

export [[nodiscard]] consteval I2sId
I2sIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "I2S2"sv) {
    return I2sId::I2s2;
  } else if (name == "I2S3"sv) {
    return I2sId::I2s3;
  }

  std::unreachable();
}

}   // namespace stm32g4