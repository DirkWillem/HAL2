module;

#include <string_view>
#include <utility>

#include <stm32g0xx_hal.h>

export module hal.stm32g0:peripherals;

import hstd;
import rtos.concepts;

namespace stm32g0 {

export enum class TimId {
  Tim1,
  Tim2,
  Tim3,
  Tim4,
  Tim6,
  Tim7,
  Tim14,
  Tim15,
  Tim16,
  Tim17
};

export [[nodiscard]] consteval TimId
TimIdFromName(std::string_view name) noexcept {
  return hstd::StaticMap<std::string_view, TimId, 10>(
      name, {
                {
                    {"TIM1", TimId::Tim1},
                    {"TIM2", TimId::Tim2},
                    {"TIM3", TimId::Tim3},
                    {"TIM4", TimId::Tim4},
                    {"TIM6", TimId::Tim6},
                    {"TIM7", TimId::Tim7},
                    {"TIM14", TimId::Tim14},
                    {"TIM15", TimId::Tim15},
                    {"TIM16", TimId::Tim16},
                    {"TIM17", TimId::Tim17},
                },
            });
}

export [[nodiscard]] constexpr TIM_TypeDef* GetTimPointer(TimId id) noexcept {
  switch (id) {
  case TimId::Tim1: return TIM1;
  case TimId::Tim2: return TIM2;
  case TimId::Tim3: return TIM3;
  case TimId::Tim4: return TIM4;
  case TimId::Tim6: return TIM6;
  case TimId::Tim7: return TIM7;
  case TimId::Tim14: return TIM14;
  case TimId::Tim15: return TIM15;
  case TimId::Tim16: return TIM16;
  case TimId::Tim17: return TIM17;
  }

  std::unreachable();
}

export enum class UartId {
  Usart1,
  Usart2,
  Usart3,
  Usart4,
  Usart5,
  Usart6,
  LpUart1,
  LpUart2,
};

export [[nodiscard]] constexpr USART_TypeDef*
GetUartPointer(UartId id) noexcept {
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

export [[nodiscard]] consteval UartId
UartIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "USART1"sv) {
    return UartId::Usart1;
  }
  if (name == "USART2"sv) {
    return UartId::Usart2;
  }
  if (name == "USART3"sv) {
    return UartId::Usart3;
  }
  if (name == "USART4"sv) {
    return UartId::Usart4;
  }
  if (name == "USART5"sv) {
    return UartId::Usart5;
  }
  if (name == "USART6"sv) {
    return UartId::Usart6;
  }
  if (name == "LPUART1"sv) {
    return UartId::LpUart1;
  }
  if (name == "LPUART2"sv) {
    return UartId::LpUart2;
  }

  std::unreachable();
}

export enum class SpiId {
  Spi1,
  Spi2,
  Spi3,
};

export [[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: return SPI1;
  case SpiId::Spi2: return SPI2;
  case SpiId::Spi3: return SPI3;
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
  }

  std::unreachable();
}

export enum class I2sId {
  I2s1,
  I2s2,
};

export [[nodiscard]] constexpr SpiId GetSpiForI2s(I2sId id) noexcept {
  switch (id) {
  case I2sId::I2s1: return SpiId::Spi1;
  case I2sId::I2s2: return SpiId::Spi2;
  }

  std::unreachable();
}

export [[nodiscard]] constexpr SPI_TypeDef* GetI2sPointer(I2sId id) noexcept {
  return GetSpiPointer(GetSpiForI2s(id));
}

export [[nodiscard]] consteval I2sId
I2sIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "I2S1"sv) {
    return I2sId::I2s1;
  } else if (name == "I2S2"sv) {
    return I2sId::I2s2;
  }

  std::unreachable();
}

export template<rtos::concepts::Rtos OS>
struct WithRtos {
  using Rtos = OS;
};

}   // namespace stm32g0
