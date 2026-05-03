#pragma once

// UART
#if defined(STM32H23xx) || defined(STM32H533xx)
#define HAS_USART_1_2_3_6
#define HAS_UART_4_5
#define HAS_LPUART_1
#elif defined(STM32H563xx) || defined(STM32H573xx)
#define HAS_USART_1_2_3_6
#define HAS_UART_4_5
#define HAS_LPUART_1

// Exclusive to STM32H563/STM32H573
#define HAS_USART_10_11
#define HAS_UART_7_8_9_12
#endif

// SPI / I2S
#if defined(STM32H23xx) || defined(STM32H533xx)
#define HAS_SPI_1_2_3
#define HAS_SPI_4
#elif defined(STM32H563xx) || defined(STM32H573xx)
#define HAS_SPI_1_2_3
#define HAS_SPI_4

// Exclusive to STM32H563/STM32H573
#define HAS_SPI_5_6
#endif

// I2C
#if defined(STM32H23xx) || defined(STM32H533xx)
#define HAS_I2C_1_2_3

#elif defined(STM32H563xx) || defined(STM32H573xx)
#define HAS_I2C_1_2_3

// Exclusive to STM32H563/STM32H573
#define HAS_I2C_4
#endif