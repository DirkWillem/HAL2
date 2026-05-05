module;

#include <array>
#include <string_view>
#include <utility>

#include <internal/peripheral_availability.h>
#include <stm32u0xx_hal.h>

export module hal.stm32u0:peripherals;

import hstd;

namespace stm32u0 {

constexpr auto PeriphName(std::string_view name, auto id) {
  return std::make_pair(name, id);
}

/** @brief USART / LPUART peripherals present on the STM32U0. */
export enum class UartId {
#ifdef HAS_USART_1_2_3_4
  Usart1,   //!< USART1.
  Usart2,   //!< USART2.
  Usart3,   //!< USART3.
  Usart4,   //!< USART4.
#endif
#ifdef HAS_LPUART_1_2
  LpUart1,   //!< LPUART1.
  LpUart2,   //!< LPUART2.
#endif
#ifdef HAS_LPUART_3
  LpUart3,   //!< LPUART3.
#endif
};

/**
 * @brief Returns a pointer to the requested UART peripheral.
 * @param uart UART peripheral to get a pointer to.
 * @return Pointer to the requested UART peripheral.
 */
export [[nodiscard]] constexpr USART_TypeDef* GetUartPointer(UartId uart) noexcept {
  using enum UartId;

  switch (uart) {
#ifdef HAS_USART_1_2_3_4
  case Usart1: return USART1;
  case Usart2: return USART2;
  case Usart3: return USART3;
  case Usart4: return USART4;
#endif
#ifdef HAS_LPUART_1_2
  case LpUart1: return LPUART1;
  case LpUart2: return LPUART2;
#endif
#ifdef HAS_LPUART_3
  case LpUart3: return LPUART3;
#endif
  }

  std::unreachable();
}

/**
 * Returns the UART ID corresponding to the given UART name.
 * @param name Name to get the corresponding UART of.
 * @return UART ID corresponding to the given name.
 */
export [[nodiscard]] consteval UartId UartIdFromName(std::string_view name) noexcept {
  using enum UartId;
  return hstd::StaticMap<std::string_view, UartId>(name, std::array{
#ifdef HAS_USART_1_2_3_4
                                                             PeriphName("USART1", Usart1),
                                                             PeriphName("USART2", Usart2),
                                                             PeriphName("USART3", Usart3),
                                                             PeriphName("USART4", Usart4),
#endif
#ifdef HAS_LPUART_1_2
                                                             PeriphName("LPUART1", LpUart1),
                                                             PeriphName("LPUART2", LpUart2),
#endif
#ifdef HAS_LPUART_3
                                                             PeriphName("LPUART3", LpUart3),
#endif
                                                         });
}

/** @brief SPI peripherals present on the STM32U0. */
export enum class SpiId {
#ifdef HAS_SPI_1_2
  Spi1,   //!< SPI1.
  Spi2,   //!< SPI2.
#endif
#ifdef HAS_SPI_3
  Spi3,   //!< SPI3.
#endif
};

/**
 * @brief I2S peripherals present on the STM32U0.
 * @note The STM32U0 does not support I2S. This enum is present purely for compatibility with
 * generated code.
 */
export enum class I2sId {};

/**
 * @brief Returns a pointer to the requested SPI peripheral.
 * @param id SPI peripheral to get a pointer to.
 * @return Pointer to the requested SPI peripheral.
 */
export [[nodiscard]] constexpr SPI_TypeDef* GetSpiPointer(SpiId id) noexcept {
  using enum SpiId;

  switch (id) {
#ifdef HAS_SPI_1_2
  case Spi1: return SPI1;
  case Spi2: return SPI2;
#endif
#ifdef HAS_SPI_3
  case Spi3: return SPI3;
#endif
  }

  std::unreachable();
}

/**
 * Returns the SPI ID corresponding to the given SPI name.
 * @param name Name to get the corresponding SPI of.
 * @return SPI ID corresponding to the given name.
 */
export [[nodiscard]] consteval SpiId SpiIdFromName(std::string_view name) noexcept {
  using enum SpiId;

  return hstd::StaticMap<std::string_view, SpiId>(name, std::array{
#ifdef HAS_SPI_1_2
                                                            PeriphName("SPI1", Spi1),
                                                            PeriphName("SPI2", Spi2),
#endif
#ifdef HAS_SPI_3
                                                            PeriphName("SPI3", Spi3),
#endif
                                                        });
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
 * @brief Returns a pointer to the requested I2C peripheral.
 * @param id I2C peripheral to get a pointer to.
 * @return Pointer to the requested I2C peripheral.
 */
export [[nodiscard]] I2C_TypeDef* GetI2cPointer(I2cId id) {
  using enum I2cId;

  switch (id) {
#ifdef HAS_I2C_1_2_3
  case I2c1: return I2C1;
  case I2c2: return I2C2;
  case I2c3: return I2C3;
#endif
#ifdef HAS_I2C_4
  case I2c4: return I2C4;
#endif
  }

  std::unreachable();
}

/**
 * Returns the I2C ID corresponding to the given I2C name.
 * @param name Name to get the corresponding I2C of.
 * @return I2C ID corresponding to the given name.
 */
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

}   // namespace stm32u0