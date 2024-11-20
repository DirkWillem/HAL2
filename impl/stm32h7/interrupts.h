#include <stm32h7xx_hal.h>

#include <stm32h7/dma.h>
#include <stm32h7/hardware_semaphore.h>
#include <stm32h7/peripheral_ids.h>
#include <stm32h7/spi.h>
#include <stm32h7/uart.h>

namespace {

template <std::size_t Id>
inline void CheckHsemInterruptCallback(uint32_t sem_mask) {
  if constexpr (hal::IsPeripheralInUse<stm32h7::HsemInterrupt<Id>>()) {
    constexpr auto IdMask = stm32h7::HsemInterrupt<Id>::IdMask;

    if ((sem_mask & IdMask) == IdMask) {
      stm32h7::HsemInterrupt<Id>::instance().FreeCallback();
    }
  }
}

template <std::size_t... Ids>
inline void CheckHsemInterruptCallbacks(uint32_t sem_mask,
                                        std::index_sequence<Ids...>) {
  (..., CheckHsemInterruptCallback<Ids>(sem_mask));
}

}   // namespace

extern "C" {

void SysTick_Handler() {
  HAL_IncTick();
}

[[noreturn]] void HardFault_Handler() {
  const volatile auto* scb = SCB;
  while (true) {}
}

static_assert(hal::Peripheral<stm32h7::Dma<stm32h7::DmaImplMarker>>);

/**
 * UART Interrupts
 */
#define UART_IRQ_HANDLER(Name)                                     \
  void Name##_IRQHandler() {                                       \
    constexpr auto Inst = stm32h7::UartIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32h7::Uart<Inst>>()) { \
      stm32h7::Uart<Inst>::instance().HandleInterrupt();           \
    }                                                              \
  }

UART_IRQ_HANDLER(USART1)
UART_IRQ_HANDLER(USART2)
UART_IRQ_HANDLER(USART3)
UART_IRQ_HANDLER(UART4)
UART_IRQ_HANDLER(UART5)
UART_IRQ_HANDLER(LPUART1)

#define HANDLE_UART_RECEIVE_CALLBACK(Inst)                 \
  if constexpr (hal::IsPeripheralInUse<stm32h7::Inst>()) { \
    if (huart == &stm32h7::Inst::instance().huart) {       \
      stm32h7::Inst::instance().ReceiveComplete(           \
          static_cast<std::size_t>(size));                 \
    }                                                      \
  }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size) {
  HANDLE_UART_RECEIVE_CALLBACK(Usart1)
  HANDLE_UART_RECEIVE_CALLBACK(Usart2)
  HANDLE_UART_RECEIVE_CALLBACK(Usart3)
  HANDLE_UART_RECEIVE_CALLBACK(Uart4)
  HANDLE_UART_RECEIVE_CALLBACK(Uart5)
  HANDLE_UART_RECEIVE_CALLBACK(LpUart1)
}

#define HANDLE_UART_TX_CALLBACK(Inst)                      \
  if constexpr (hal::IsPeripheralInUse<stm32h7::Inst>()) { \
    if (huart == &stm32h7::Inst::instance().huart) {       \
      stm32h7::Inst::instance().TransmitComplete();        \
    }                                                      \
  }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  HANDLE_UART_TX_CALLBACK(Usart1)
  HANDLE_UART_TX_CALLBACK(Usart2)
  HANDLE_UART_TX_CALLBACK(Usart3)
  HANDLE_UART_TX_CALLBACK(Uart4)
  HANDLE_UART_TX_CALLBACK(Uart5)
  HANDLE_UART_TX_CALLBACK(LpUart1)
}

/**
 * SPI Interrupts
 */

#define SPI_IRQ_HANDLER(Name)                                     \
  void Name##_IRQHandler() {                                      \
    constexpr auto Inst = stm32h7::SpiIdFromName(#Name);          \
    if constexpr (hal::IsPeripheralInUse<stm32h7::Spi<Inst>>()) { \
      stm32h7::Spi<Inst>::instance().HandleInterrupt();           \
    }                                                             \
  }

SPI_IRQ_HANDLER(SPI1)
SPI_IRQ_HANDLER(SPI2)
SPI_IRQ_HANDLER(SPI3)
SPI_IRQ_HANDLER(SPI4)
SPI_IRQ_HANDLER(SPI5)
SPI_IRQ_HANDLER(SPI6)

#define HANDLE_SPI_RX_CALLBACK(Inst)                       \
  if constexpr (hal::IsPeripheralInUse<stm32h7::Inst>()) { \
    if (hspi == &stm32h7::Inst::instance().hspi) {         \
      stm32h7::Inst::instance().RxComplete();              \
    }                                                      \
  }

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi) {
  HANDLE_SPI_RX_CALLBACK(Spi1)
  HANDLE_SPI_RX_CALLBACK(Spi2)
  HANDLE_SPI_RX_CALLBACK(Spi3)
  HANDLE_SPI_RX_CALLBACK(Spi4)
  HANDLE_SPI_RX_CALLBACK(Spi5)
  HANDLE_SPI_RX_CALLBACK(Spi6)
}

#define HANDLE_SPI_TX_CALLBACK(Inst)                       \
  if constexpr (hal::IsPeripheralInUse<stm32h7::Inst>()) { \
    if (hspi == &stm32h7::Inst::instance().hspi) {         \
      stm32h7::Inst::instance().TxComplete();              \
    }                                                      \
  }

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
  HANDLE_SPI_TX_CALLBACK(Spi1)
  HANDLE_SPI_TX_CALLBACK(Spi2)
  HANDLE_SPI_TX_CALLBACK(Spi3)
  HANDLE_SPI_TX_CALLBACK(Spi4)
  HANDLE_SPI_TX_CALLBACK(Spi5)
  HANDLE_SPI_TX_CALLBACK(Spi6)
}

#define DMA_IRQ_HANDLER(Inst, Chan)                                            \
  void DMA##Inst##_Stream##Chan##_IRQHandler() {                               \
    if constexpr (hal::IsPeripheralInUse<                                      \
                      stm32h7::Dma<stm32h7::DmaImplMarker>>()) {               \
      if (stm32h7::Dma<stm32h7::DmaImplMarker>::DmaChannelInUseForCurrentCore< \
              Inst, Chan>()) {                                                 \
        stm32h7::Dma<stm32h7::DmaImplMarker>::instance()                       \
            .HandleDmaInterrupt<Inst, Chan>();                                 \
      }                                                                        \
    }                                                                          \
  }

DMA_IRQ_HANDLER(1, 0)
DMA_IRQ_HANDLER(1, 1)
DMA_IRQ_HANDLER(1, 2)
DMA_IRQ_HANDLER(1, 3)
DMA_IRQ_HANDLER(1, 4)
DMA_IRQ_HANDLER(1, 5)
DMA_IRQ_HANDLER(1, 6)
DMA_IRQ_HANDLER(1, 7)

DMA_IRQ_HANDLER(2, 0)
DMA_IRQ_HANDLER(2, 1)
DMA_IRQ_HANDLER(2, 2)
DMA_IRQ_HANDLER(2, 3)
DMA_IRQ_HANDLER(2, 4)
DMA_IRQ_HANDLER(2, 5)
DMA_IRQ_HANDLER(2, 6)
DMA_IRQ_HANDLER(2, 7)

#define BDMA_IRQ_HANDLER(Inst, Chan)                             \
  void BDMA_Channel##Chan##_IRQHandler() {                       \
    if constexpr (hal::IsPeripheralInUse<                        \
                      stm32h7::Dma<stm32h7::DmaImplMarker>>()) { \
      if (stm32h7::Dma<stm32h7::DmaImplMarker>::                 \
              BdmaChannelInUseForCurrentCore<Inst, Chan>()) {    \
        stm32h7::Dma<stm32h7::DmaImplMarker>::instance()         \
            .HandleBdmaInterrupt<Inst, Chan>();                  \
      }                                                          \
    }                                                            \
  }

BDMA_IRQ_HANDLER(1, 0)
BDMA_IRQ_HANDLER(1, 1)
BDMA_IRQ_HANDLER(1, 2)
BDMA_IRQ_HANDLER(1, 3)
BDMA_IRQ_HANDLER(1, 4)
BDMA_IRQ_HANDLER(1, 5)
BDMA_IRQ_HANDLER(1, 6)
BDMA_IRQ_HANDLER(1, 7)

#if defined(CORE_CM7)
void HSEM1_IRQHandler() {
  HAL_HSEM_IRQHandler();
}
#endif

#if defined(CORE_CM4)
void HSEM2_IRQHandler() {
  HAL_HSEM_IRQHandler();
}
#endif

void HAL_HSEM_FreeCallback(uint32_t sem_mask) {
  CheckHsemInterruptCallbacks(sem_mask, std::make_index_sequence<32>());
}
}
