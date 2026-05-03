module;

#include <chrono>
#include <utility>

#include <stm32h5xx_hal.h>

#include <internal/peripheral_availability.h>

export module hal.stm32h5:peripherals;

import hstd;

namespace stm32h5 {

export enum class UartId {
#ifdef HAS_USART_1_2_3_6
  Usart1,
  Usart2,
  Usart3,
  Usart6,
#endif
#ifdef HAS_UART_4_5
  Uart4,
  Uart5,
#endif
#ifdef HAS_USART_10_11
  Usart10,
  Usart11,
#endif
#ifdef HAS_UART_7_8_9_12
  Uart7,
  Uart8,
  Uart9,
  Uart12,
#endif
#ifdef HAS_LPUART_1
  LpUart1,
#endif
};

export [[nodiscard]] constexpr USART_TypeDef* GetUartPointer(UartId uart) noexcept {
  using enum UartId;

  switch (uart) {
#ifdef HAS_USART_1_2_3_6
  case Usart1: return USART1;
  case Usart2: return USART2;
  case Usart3: return USART3;
  case Usart6: return USART6;
#endif
#ifdef HAS_UART_4_5
  case Uart4: return UART4;
  case Uart5: return UART5;
#endif
#ifdef HAS_USART_10_11
  case Usart10: return USART10;
  case Usart11: return USART11;
#endif
#ifdef HAS_UART_7_8_9_12
  case Uart7: return UART7;
  case Uart8: return UART8;
  case Uart9: return UART9;
  case Uart12: return UART12;
#endif
#ifdef HAS_LPUART_1
  case LpUart1: return LPUART1;
#endif
  }

  std::unreachable();
}

constexpr auto PeriphName(std::string_view name, auto id) {
  return std::make_pair(name, id);
}

export [[nodiscard]] consteval UartId UartIdFromName(std::string_view name) noexcept {
  using enum UartId;
  return hstd::StaticMap<std::string_view, UartId>(name, std::array{
#ifdef HAS_USART_1_2_3_6
                                                             PeriphName("USART1", Usart1),
                                                             PeriphName("USART2", Usart2),
                                                             PeriphName("USART3", Usart3),
                                                             PeriphName("USART6", Usart6),
#endif
#ifdef HAS_UART_4_5
                                                             PeriphName("UART4", Uart4),
                                                             PeriphName("UART5", Uart5),
#endif
#ifdef HAS_LPUART_1
                                                             PeriphName("LPUART1", LpUart1),
#endif
#ifdef HAS_USART_10_11
                                                             PeriphName("USART10", Usart10),
                                                             PeriphName("USART11", Usart11),
#endif
#ifdef HAS_UART_7_8_9_12
                                                             PeriphName("UART7", Uart7),
                                                             PeriphName("UART8", Uart8),
                                                             PeriphName("UART9", Uart9),
                                                             PeriphName("UART12", Uart12),
#endif
                                                         });
}

export enum class SpiId {
#ifdef HAS_SPI_1_2_3
  Spi1,
  Spi2,
  Spi3,
#endif
#ifdef HAS_SPI_4
  Spi4,
#endif
#ifdef HAS_SPI_5_6
  Spi5,
  Spi6,
#endif
};

export [[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  using enum SpiId;

  switch (id) {
#ifdef HAS_SPI_1_2_3
  case Spi1: return SPI1;
  case Spi2: return SPI2;
  case Spi3: return SPI3;
#endif
#ifdef HAS_SPI_4
  case Spi4: return SPI4;
#endif
#ifdef HAS_SPI_5_6
  case Spi5: return SPI5;
  case Spi6: return SPI6;
#endif
  }

  std::unreachable();
}

export [[nodiscard]] consteval SpiId SpiIdFromName(std::string_view name) noexcept {
  using enum SpiId;

  return hstd::StaticMap<std::string_view, SpiId>(name, std::array{
#ifdef HAS_SPI_1_2_3
                                                            PeriphName("SPI1", Spi1),
                                                            PeriphName("SPI2", Spi2),
                                                            PeriphName("SPI3", Spi3),
#endif
#ifdef HAS_SPI_4
                                                            PeriphName("SPI4", Spi4),
#endif
#ifdef HAS_SPI_5_6
                                                            PeriphName("SPI5", Spi5),
                                                            PeriphName("SPI6", Spi6),
#endif
                                                        });
}

export enum class I2sId { I2s1, I2s2, I2s3 };

export [[nodiscard]] consteval I2sId I2sIdFromName(std::string_view name) noexcept {
  using enum I2sId;

  return hstd::StaticMap<std::string_view, I2sId>(name, std::array{
#ifdef HAS_SPI_1_2_3
                                                            PeriphName("I2S1", I2s1),
                                                            PeriphName("I2S2", I2s2),
                                                            PeriphName("I2S3", I2s3),
#endif
                                                        });
}

export [[nodiscard]] consteval SpiId GetSpiForI2s(I2sId id) {
  using enum SpiId;
  using enum I2sId;

  switch (id) {
#ifdef HAS_SPI_1_2_3
  case I2s1: return Spi1;
  case I2s2: return Spi2;
  case I2s3: return Spi3;
#endif
  }

  std::unreachable();
}

export enum class TimId { Tim1, Tim2, Tim3, Tim4, Tim5, Tim6, Tim7, Tim8, Tim12, Tim15 };

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

/** @brief I2C peripherals present in the STM32H5 */
export enum class I2cId {
#ifdef HAS_I2C_1_2_3
  I2c1,   //!< I2C1.
  I2c2,   //!< I2C2.
  I2c3,   //!< I2C3.
#endif
#ifdef HAS_I2C_4
  I2c4,   //!< I2C4.
#endif
};

/**
 * @brief Returns a pointer to the requested I2C instance.
 * @param id ID of the I2C to get a pointer to.
 * @return \c I2C_TypeDef pointer to the requested I2C instance.
 */
export [[nodiscard]] I2C_TypeDef* GetI2cPointer(I2cId id) {
  using enum I2cId;

  switch (id) {
  case I2c1: return I2C1;
  case I2c2: return I2C2;
  case I2c3: return I2C3;
  }

  std::unreachable();
}

export [[nodiscard]] consteval I2cId I2cIdFromName(std::string_view name) noexcept {
  using enum I2cId;

  return hstd::StaticMap<std::string_view, I2cId>(name, std::array{
#ifdef HAS_I2C_1_2_3
                                                            PeriphName("I2C1", I2c1),
                                                            PeriphName("I2C2", I2c2),
                                                            PeriphName("I2C3", I2c3),
#endif
#ifdef HAS_I2C_4
                                                            PeriphName("I2C4", I2c4),
#endif
                                                        });
}

}   // namespace stm32h5