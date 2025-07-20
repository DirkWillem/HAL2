#pragma once

/**
 * Clocks
 */
#if defined(STM32G0X0)
// PLLQ
#if defined(STM32G0B0)
#define HAS_PLLQ
#endif

// USBCPD Strobe bits
#if defined(STM32G070) || defined(STM32G0B0)
#define HAS_USBCPD_STROBE_BITS
#endif
#elif defined(STM32G0X1)
#define HAS_PLLQ

// USBCPD Strobe bits
#if defined(STM32G071) || defined(STM32G091) || defined(STM32G0B1) \
    || defined(STM32G0C1)
#define HAS_USBCPD_STROBE_BITS
#endif

// HSI48
#if defined(STM32G0B1) || defined(STM32G0C1)
#define HAS_HSI48
#endif
#endif

/**
 * DMA
 */
#if defined(STM32G0X0)
#if defined(STM32G030)
#define N_DMA1_CHANS 5
#endif
#if defined(STM32G050) || defined(STM32G070)
#define N_DMA1_CHANS 7
#endif
#if defined(STM32G0B0)
#define HAS_DMA2

#define N_DMA1_CHANS 7
#define N_DMA2_CHANS 5
#endif
#elif defined(STM32G0X1)
#if defined(STM32G031) || defined(STM32G041)
#define N_DMA1_CHANS 5
#endif
#if defined(STM32G051) || defined(STM32G061) || defined(STM32G071) \
    || defined(STM32G081)
#define N_DMA1_CHANS 7
#endif
#if defined(STM32G0B1) || defined(STM32G0C1)
#define HAS_DMA2

#define N_DMA1_CHANS 7
#define N_DMA2_CHANS 5
#endif
#endif

/**
 * TIMERS
 */
#if defined(STM32G0X0)

// TIM4
#if defined(STM32G0B0)
#define HAS_TIM4
#endif
// TIM6, TIM7
#if defined(STM32G050) || defined(STM32G070) || defined(STM32G0B0)
#define HAS_TIM67
#endif
// TIM15
#if defined(STM32G070) || defined(STM32G0B1)
#define HAS_TIM15
#endif

#elif defined(STM32G0X1)

#define HAS_TIM2

// TIM4
#if defined(STM32G0B1) || defined(STM32G0C1)
#define HAS_TIM4
#endif
// TIM6, TIM7 and TIM15
#if defined(STM32G051) || defined(STM32G061) || defined(STM32G071) \
    || defined(STM32G081) || defined(STM32G0B1) || defined(STM32G0C1)
#define HAS_TIM67
#define HAS_TIM15
#endif

/**
 * UART / USART / LPUART
 */
#if defined(STM32G0X0)

// USART3, USART4
#if defined(STM32G070) || defined(STM32G0B0)
#define HAS_USART34
#endif

// USART5, USART6
#if defined(STM32G0B0)
#define HAS_USART56
#endif
#elif defined(STM32G0X1)

// USART3, USART4
#if defined(STM32G071) || defined(STM32G081) || defined(STM32G0B1) \
    || defined(STM32G0C1)
#define HAS_USART34
#endif

// USART5, USART6
#if defined(STM32G0B1) || defined(STM32G0C1)
#define HAS_USART56
#endif

#define HAS_LPUART1

// LPUART2
#if defined(STM32G0B1) || defined(STM32G0C1)
#define HAS_LPUART2
#endif

#endif

/**
 * SPI
 */
#if (defined(STM32G0x0) && defined(STM32G0B0)) || defined(STM32G0X1)
#define HAS_SPI3
#endif

/**
 * FDCAN
 */
#if defined(STM32G0X1) && (defined(STM32G0B1) || defined(STM32G0C1))
#define HAS_FDCAN12
#endif

#endif