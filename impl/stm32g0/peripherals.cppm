module;

#include <string_view>
#include <utility>

#include <stm32g0xx_hal.h>

#include "internal/peripheral_availability.h"

export module hal.stm32g0:peripherals;

import hstd;
import rtos.concepts;

namespace stm32g0 {

export enum class TimId {
  Tim1,
#ifdef HAS_TIM2
  Tim2,
#endif
  Tim3,
#ifdef HAS_TIM4
  Tim4,
#endif
#ifdef HAS_TIM67
  Tim6,
  Tim7,
#endif
  Tim14,
#ifdef HAS_TIM15
  Tim15,
#endif
  Tim16,
  Tim17,
  Invalid,
};

export [[nodiscard]] consteval TimId
TimIdFromName(std::string_view name) noexcept {
  using enum TimId;

  if (name == "TIM1") {
    return Tim1;
  }
#ifdef HAS_TIM2
  if (name == "TIM2") {
    return Tim2;
  }
#endif
  if (name == "TIM3") {
    return Tim3;
  }
#ifdef HAS_TIM4
  if (name == "TIM4") {
    return Tim4;
  }
#endif
#ifdef HAS_TIM67
  if (name == "TIM6") {
    return Tim6;
  }
  if (name == "TIM7") {
    return Tim7;
  }
#endif
  if (name == "TIM14") {
    return Tim14;
  }
#ifdef HAS_TIM15
  if (name == "TIM15") {
    return Tim15;
  }
#endif
  if (name == "TIM16") {
    return Tim16;
  }
  if (name == "TIM17") {
    return Tim17;
  }

  // Special case for STM32G0 with TIM2
#if defined(STM32G0X0)
  if (name == "TIM2") {
    return Invalid;
  }
#endif

  std::unreachable();
}

export [[nodiscard]] constexpr TIM_TypeDef* GetTimPointer(TimId id) noexcept {
  switch (id) {
  case TimId::Tim1: return TIM1;
#ifdef HAS_TIM2
  case TimId::Tim2: return TIM2;
#endif
  case TimId::Tim3: return TIM3;
#ifdef HAS_TIM4
  case TimId::Tim4: return TIM4;
#endif
#ifdef HAS_TIM67
  case TimId::Tim6: return TIM6;
  case TimId::Tim7: return TIM7;
#endif
  case TimId::Tim14: return TIM14;
#ifdef HAS_TIM15
  case TimId::Tim15: return TIM15;
#endif
  case TimId::Tim16: return TIM16;
  case TimId::Tim17: return TIM17;
  }

  std::unreachable();
}



export enum class UartId {
  Usart1,
  Usart2,
#ifdef HAS_USART34
  Usart3,
  Usart4,
#endif
#ifdef HAS_USART56
  Usart5,
  Usart6,
#endif
#ifdef HAS_LPUART1
  LpUart1,
#endif
#ifdef HAS_LPUART2
  LpUart2,
#endif
  Invalid,
};

export [[nodiscard]] constexpr USART_TypeDef*
GetUartPointer(UartId id) noexcept {
  switch (id) {
  case UartId::Usart1: return USART1;
  case UartId::Usart2: return USART2;
#ifdef HAS_USART34
  case UartId::Usart3: return USART3;
  case UartId::Usart4: return USART4;
#endif
#ifdef HAS_USART56
  case UartId::Usart5: return USART5;
  case UartId::Usart6: return USART6;
#endif
#ifdef HAS_LPUART1
  case UartId::LpUart1: return LPUART1;
#endif
#ifdef HAS_LPUART2
  case UartId::LpUart2: return LPUART2;
#endif
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
#ifdef HAS_USART34
  if (name == "USART3"sv) {
    return UartId::Usart3;
  }
  if (name == "USART4"sv) {
    return UartId::Usart4;
  }
#endif
#ifdef HAS_USART56
  if (name == "USART5"sv) {
    return UartId::Usart5;
  }
  if (name == "USART6"sv) {
    return UartId::Usart6;
  }
#endif
#ifdef HAS_LPUART1
  if (name == "LPUART1"sv) {
    return UartId::LpUart1;
  }
#endif
#ifdef HAS_LPUART2
  if (name == "LPUART2"sv) {
    return UartId::LpUart2;
  }
#endif

  // Special case for STM32G0 with LPUART1
#if defined(STM32G0X0)
  if (name == "LPUART1"sv) {
    return UartId::Invalid;
  }
#endif

  std::unreachable();
}

export enum class SpiId {
  Spi1,
  Spi2,
#ifdef HAS_SPI3
  Spi3,
#endif
};

export [[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: return SPI1;
  case SpiId::Spi2: return SPI2;
#ifdef HAS_SPI3
  case SpiId::Spi3: return SPI3;
#endif
  }

  std::unreachable();
}

export [[nodiscard]] consteval SpiId
SpiIdFromName(std::string_view name) noexcept {
  using std::operator""sv;
  if (name == "SPI1"sv) {
    return SpiId::Spi1;
  }
  if (name == "SPI2"sv) {
    return SpiId::Spi2;
  }

#ifdef HAS_SPI3
  if (name == "SPI3"sv) {
    return SpiId::Spi3;
  }
#endif

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
  }
  if (name == "I2S2"sv) {
    return I2sId::I2s2;
  }

  std::unreachable();
}

export template <rtos::concepts::Rtos OS>
struct WithRtos {
  using Rtos = OS;
};

}   // namespace stm32g0
