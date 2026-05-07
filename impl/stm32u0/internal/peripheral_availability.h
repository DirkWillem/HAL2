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

// DMA
#if defined(STM32031xx)
#define N_DMA1_CHANS (7)
#define N_DMA2_CHANS (0)
#elif defined(STM32U073xx) || defined(STM32U083xx)
#define N_DMA1_CHANS (7)
#define N_DMA2_CHANS (5)
#endif