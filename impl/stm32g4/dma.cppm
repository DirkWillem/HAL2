module;

#include <array>
#include <cstdint>
#include <utility>

#include <stm32g4xx_hal.h>

export module hal.stm32g4:dma;

import hal.abstract;

import :peripherals;

namespace stm32g4 {

/**
 * Type alias for a DMA channel list
 * @tparam Channels DMA channels
 */
export template <hal::DmaChannel... Channels>
using DmaChannels = hal::DmaChannels<Channels...>;

/** Possible DMA requests for UART */
export enum class UartDmaRequest { Tx, Rx };

/** Possible DMA requests for I2C */
export enum class I2cDmaRequest { Tx, Rx };

/** Possible DMA requests for SPI */
export enum class SpiDmaRequest { Tx, Rx };

/** Possible DMA requests for TIM */
export enum class TimDmaRequest { PeriodElapsed, Ch1, Ch2, Ch3, Ch4 };

[[nodiscard]] uint32_t GetDmaRequestId(UartId         id,
                                       UartDmaRequest request) noexcept {
  switch (request) {
  case UartDmaRequest::Tx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_TX;
    case UartId::Usart2: return DMA_REQUEST_USART2_TX;
    case UartId::Usart3: return DMA_REQUEST_USART3_TX;
    case UartId::Uart4: return DMA_REQUEST_UART4_TX;
    case UartId::Uart5: return DMA_REQUEST_UART5_TX;
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_TX;
    }
    std::unreachable();
  case UartDmaRequest::Rx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_RX;
    case UartId::Usart2: return DMA_REQUEST_USART2_RX;
    case UartId::Usart3: return DMA_REQUEST_USART3_RX;
    case UartId::Uart4: return DMA_REQUEST_UART4_RX;
    case UartId::Uart5: return DMA_REQUEST_UART5_RX;
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_RX;
    }
    std::unreachable();
  }
  std::unreachable();
}

[[nodiscard]] uint32_t GetDmaRequestId(I2cId         id,
                                       I2cDmaRequest request) noexcept {
  switch (request) {
  case I2cDmaRequest::Tx:
    switch (id) {
    case I2cId::I2c1: return DMA_REQUEST_I2C1_TX;
    case I2cId::I2c2: return DMA_REQUEST_I2C2_TX;
    case I2cId::I2c3: return DMA_REQUEST_I2C3_TX;
    case I2cId::I2c4: return DMA_REQUEST_I2C4_TX;
    default: std::unreachable();
    }
  case I2cDmaRequest::Rx:
    switch (id) {
    case I2cId::I2c1: return DMA_REQUEST_I2C1_RX;
    case I2cId::I2c2: return DMA_REQUEST_I2C2_RX;
    case I2cId::I2c3: return DMA_REQUEST_I2C3_RX;
    case I2cId::I2c4: return DMA_REQUEST_I2C4_RX;
    default: std::unreachable();
    }
  default: std::unreachable();
  }
}

[[nodiscard]] uint32_t GetDmaRequestId(SpiId         id,
                                       SpiDmaRequest request) noexcept {
  switch (request) {
  case SpiDmaRequest::Tx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_TX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_TX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_TX;
    }
    std::unreachable();
  case SpiDmaRequest::Rx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_RX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_RX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_RX;
    }
    std::unreachable();
  }
  std::unreachable();
}

// [[nodiscard]] uint32_t GetDmaRequestId(TimId         id,
//                                        TimDmaRequest request) noexcept {
//   switch (id) {
//   case TimId::Tim1:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM1_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM1_CH1;
//     case TimDmaRequest::Ch2: return DMA_REQUEST_TIM1_CH2;
//     case TimDmaRequest::Ch3: return DMA_REQUEST_TIM1_CH3;
//     case TimDmaRequest::Ch4: return DMA_REQUEST_TIM1_CH4;
//     default: std::unreachable();
//     }
//   case TimId::Tim2:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM2_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM2_CH1;
//     case TimDmaRequest::Ch2: return DMA_REQUEST_TIM2_CH2;
//     case TimDmaRequest::Ch3: return DMA_REQUEST_TIM2_CH3;
//     case TimDmaRequest::Ch4: return DMA_REQUEST_TIM2_CH4;
//     default: std::unreachable();
//     }
//   case TimId::Tim3:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM3_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM3_CH1;
//     case TimDmaRequest::Ch2: return DMA_REQUEST_TIM3_CH2;
//     case TimDmaRequest::Ch3: return DMA_REQUEST_TIM3_CH3;
//     case TimDmaRequest::Ch4: return DMA_REQUEST_TIM3_CH4;
//     default: std::unreachable();
//     }
//   case TimId::Tim4:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM4_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM4_CH1;
//     case TimDmaRequest::Ch2: return DMA_REQUEST_TIM4_CH2;
//     case TimDmaRequest::Ch3: return DMA_REQUEST_TIM4_CH3;
//     case TimDmaRequest::Ch4: return DMA_REQUEST_TIM4_CH4;
//     default: std::unreachable();
//     }
//   case TimId::Tim15:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM15_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM15_CH1;
//     case TimDmaRequest::Ch2: return DMA_REQUEST_TIM15_CH2;
//     default: std::unreachable();
//     }
//   case TimId::Tim16:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM16_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM16_CH1;
//     default: std::unreachable();
//     }
//   case TimId::Tim17:
//     switch (request) {
//     case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM17_UP;
//     case TimDmaRequest::Ch1: return DMA_REQUEST_TIM17_CH1;
//     default: std::unreachable();
//     }
//   default: break;
//   }
//
//   std::unreachable();
// }

[[nodiscard]] uint32_t ToHalDmaDirection(hal::DmaDirection dir) noexcept {
  switch (dir) {
  case hal::DmaDirection::MemToPeriph: return DMA_MEMORY_TO_PERIPH;
  case hal::DmaDirection::PeriphToMem: return DMA_PERIPH_TO_MEMORY;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t ToHalDmaMode(hal::DmaMode mode) noexcept {
  switch (mode) {
  case hal::DmaMode::Normal: return DMA_NORMAL;
  case hal::DmaMode::Circular: return DMA_CIRCULAR;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalMemDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_MDATAALIGN_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_MDATAALIGN_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_MDATAALIGN_WORD;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalPeriphDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_PDATAALIGN_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_PDATAALIGN_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_PDATAALIGN_WORD;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t ToHalDmaPriority(hal::DmaPriority prio) {
  switch (prio) {
  case hal::DmaPriority::Low: return DMA_PRIORITY_LOW;
  case hal::DmaPriority::Medium: return DMA_PRIORITY_MEDIUM;
  case hal::DmaPriority::High: return DMA_PRIORITY_HIGH;
  case hal::DmaPriority::VeryHigh: return DMA_PRIORITY_VERY_HIGH;
  }

  std::unreachable();
}

void SetupDma(std::size_t n_used_channels) noexcept {
#if defined(STM32G474xx)
  // Enable clocks
  if (n_used_channels > 0) {
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
  }

  if (n_used_channels > 8) {
    __HAL_RCC_DMA2_CLK_ENABLE();
  }

  // Enable IRQ
  static constexpr std::array<IRQn_Type, 16> ChannelIrqns{
      DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn,
      DMA1_Channel4_IRQn, DMA1_Channel5_IRQn, DMA1_Channel6_IRQn,
      DMA1_Channel7_IRQn, DMA1_Channel8_IRQn, DMA2_Channel1_IRQn,
      DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn,
      DMA2_Channel5_IRQn, DMA2_Channel6_IRQn, DMA2_Channel7_IRQn,
      DMA2_Channel8_IRQn,
  };

  for (std::size_t i = 0; i < n_used_channels; i++) {
    HAL_NVIC_SetPriority(ChannelIrqns[i], 0, 0);
    HAL_NVIC_EnableIRQ(ChannelIrqns[i]);
  }
#else
#error "Cannot determine DMA interrupts for this STM32G4 variant"
#endif
}

/**
 * DMA channel ID
 * @tparam Periph Channel peripheral
 * @tparam Req Channel request type
 */
export template <auto Periph, auto Req>
struct DmaChannelId {
  static constexpr auto Peripheral = Periph;
  static constexpr auto Request    = Req;
};

/**
 * DMA channel definition
 * @tparam Periph Channel peripheral
 * @tparam Req Channel request type
 * @tparam Prio Channel priority
 */
export template <auto Periph, auto Req,
                 hal::DmaPriority Prio = hal::DmaPriority::Low>
struct DmaChannel {
  static constexpr auto Peripheral = Periph;
  static constexpr auto Request    = Req;
  static constexpr auto Priority   = Prio;
};

export template <typename Impl, typename Channels>
class DmaImpl;

export template <typename Impl, hal::DmaChannel... Channels>
class DmaImpl<Impl, hal::DmaChannels<Channels...>>
    : public hal::UsedPeripheral {
#if defined(STM32G474xx)
  static constexpr auto NDma1Channels = 8u;
  static constexpr auto NDma2Channels = 8u;
#else
#error "Cannot determine number of DMA channels for this STM32G4 variant"
#endif

  static constexpr auto NMaxDmaChannels = NDma1Channels + NDma2Channels;

  using ChanList = hal::DmaChannels<Channels...>;

 public:
  static Impl& instance() {
    static Impl inst{};
    return inst;
  }

  template <hal::DmaChannelId Chan>
  /**
   * Returns whether the given channel is enabled
   * @tparam Chan Channel to check
   * @return Whether Chan is enabled
   */
  [[nodiscard]] static constexpr bool ChannelEnabled() noexcept {
    return ChanList::template ContainsChanId<Chan>();
  }

  template <unsigned DmaInst, unsigned Chan>
    requires(Chan >= 1
             && ((DmaInst == 1 && Chan <= NDma1Channels)
                 || (DmaInst == 2 && Chan <= NDma2Channels)))
  [[nodiscard]] static consteval bool ChannelInUse() noexcept {
    if constexpr (DmaInst == 1) {
      constexpr auto Idx = Chan - 1;
      return Idx < sizeof...(Channels);
    } else if constexpr (DmaInst == 2) {
      constexpr auto Idx = Chan + NDma1Channels - 1;
      return Idx < sizeof...(Channels);
    } else {
      std::unreachable();
    }
  }

  template <hal::DmaChannelId Chan>
  /**
   * Sets up a DMA channel
   * @tparam Chan Channel to set up
   * @param dir Channel direction
   * @param mode Channel mode
   * @param periph_data_width Peripheral data width
   * @param mem_data_width Memory data width
   * @return Reference to the DMA handle
   */
  [[nodiscard]] DMA_HandleTypeDef&
  SetupChannel(hal::DmaDirection dir, hal::DmaMode mode,
               hal::DmaDataWidth periph_data_width, bool periph_inc,
               hal::DmaDataWidth mem_data_width, bool mem_inc) noexcept {
    constexpr auto Idx  = ChanList::template DmaChannelIndex<Chan>();
    auto&          hdma = hdmas[Idx];

    hdma.Instance = DmaChannel<Chan>();
    hdma.Init     = {
            .Request             = GetDmaRequestId(Chan::Peripheral, Chan::Request),
            .Direction           = ToHalDmaDirection(dir),
            .PeriphInc           = periph_inc ? DMA_PINC_ENABLE : DMA_PINC_DISABLE,
            .MemInc              = mem_inc ? DMA_MINC_ENABLE : DMA_MINC_DISABLE,
            .PeriphDataAlignment = ToHalPeriphDataWidth(periph_data_width),
            .MemDataAlignment    = ToHalMemDataWidth(mem_data_width),
            .Mode                = ToHalDmaMode(mode),
            .Priority =
            ToHalDmaPriority(ChanList::template DmaChannelPriority<Chan>()),
    };
    HAL_DMA_Init(&hdma);

    return hdma;
  }

  template <unsigned DmaInst, unsigned Chan>
    requires(Chan >= 1
             && ((DmaInst == 1 && Chan <= NDma1Channels)
                 || (DmaInst == 2 && Chan <= NDma2Channels)))
  /**
   * DMA interrupt handler
   * @tparam ChanIdx DMA channel index
   */
  void HandleInterrupt() noexcept {
    if constexpr (DmaInst == 1) {
      HAL_DMA_IRQHandler(&hdmas[Chan - 1]);
    } else if constexpr (DmaInst == 2) {
      HAL_DMA_IRQHandler(&hdmas[Chan + NDma1Channels - 1]);
    } else {
      std::unreachable();
    }
  }

 private:
  DmaImpl() { SetupDma(sizeof...(Channels)); }

  template <hal::DmaChannelId Chan>
    requires(ChanList::template ContainsChanId<Chan>())
  [[nodiscard]] static constexpr DMA_Channel_TypeDef* DmaChannel() noexcept {
    const std::array<DMA_Channel_TypeDef*, NMaxDmaChannels> channels{
        DMA1_Channel1, DMA1_Channel2, DMA1_Channel3, DMA1_Channel4,
        DMA1_Channel5, DMA1_Channel6, DMA1_Channel7, DMA1_Channel8,
        DMA2_Channel1, DMA2_Channel2, DMA2_Channel3, DMA2_Channel4,
        DMA2_Channel5, DMA2_Channel6, DMA2_Channel7, DMA2_Channel8};

    return channels[ChanList::template DmaChannelIndex<Chan>()];
  }

  std::array<DMA_HandleTypeDef, sizeof...(Channels)> hdmas{};
};

export struct DmaImplMarker {};

export template <typename M>
class Dma : public hal::UnusedPeripheral<Dma<M>> {
 public:
  template <unsigned DmaInst, unsigned Chan>
  [[nodiscard]] static constexpr bool ChannelInUse() noexcept {
    std::unreachable();
    return false;
  }

  template <unsigned DmaInst, unsigned Chan>
  constexpr void HandleInterrupt() noexcept {
    std::unreachable();
  }
};

}   // namespace stm32g4