#pragma once

// UART
#if defined(STM32U031xx)
#define HAS_USART_1_2_3_4
#define HAS_LPUART_1_2
#elif defined(STM32U073xx) || defined(STM32U083xx)
#define HAS_USART_1_2_3_4
#define HAS_LPUART_1_2

// Exclusive to STM32U073xx/STM32U083xx
#define HAS_LPUART_3
#endif

// SPI / I2S
#if defined(STM32U031xx)
#define HAS_SPI_1_2
#elif defined(STM32U073xx) || defined(STM32U083xx)
#define HAS_SPI_1_2

// Exclusive to STM32U073xx/STM32U083xx
#define HAS_SPI_3
#endif

// I2C
#if defined(STM32U031xx)
#define HAS_I2C_1_2_3
#elif defined(STM32U073xx) || defined(STM32U083xx)
#define HAS_I2C_1_2_3

// Exclusive to STM32U073xx/STM32U083xx
#define HAS_I2C_4
#endif
