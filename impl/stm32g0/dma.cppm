module;

#include <array>
#include <cstdint>
#include <utility>

#include <stm32g0xx_hal.h>

#include "internal/peripheral_availability.h"

export module hal.stm32g0:dma;

import hal.abstract;

import :peripherals;

namespace stm32g0 {

inline constexpr auto NDma1Chans = N_DMA1_CHANS;

#ifdef N_DMA2_CHANS
inline constexpr auto NDma2Chans = N_DMA2_CHANS;
#else
inline constexpr auto NDma2Chans = 0;
#endif

/**
 * Type alias for a DMA channel list
 * @tparam Channels DMA channels
 */
export template <hal::DmaChannel... Channels>
using DmaChannels = hal::DmaChannels<Channels...>;

/** Possible DMA requests for UART */
export enum class UartDmaRequest { Tx, Rx };

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
#ifdef HAS_USART34
    case UartId::Usart3: return DMA_REQUEST_USART3_TX;
    case UartId::Usart4: return DMA_REQUEST_USART4_TX;
#endif
#ifdef HAS_USART56
    case UartId::Usart5: return DMA_REQUEST_USART5_TX;
    case UartId::Usart6: return DMA_REQUEST_USART6_TX;
#endif
#ifdef HAS_LPUART1
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_TX;
#endif
#ifdef HAS_LPUART2
    case UartId::LpUart2: return DMA_REQUEST_LPUART2_TX;
#endif
    }
    std::unreachable();
  case UartDmaRequest::Rx:
    switch (id) {
    case UartId::Usart1: return DMA_REQUEST_USART1_RX;
    case UartId::Usart2: return DMA_REQUEST_USART2_RX;
#ifdef HAS_USART34
    case UartId::Usart3: return DMA_REQUEST_USART3_RX;
    case UartId::Usart4: return DMA_REQUEST_USART4_RX;
#endif
#ifdef HAS_USART56
    case UartId::Usart5: return DMA_REQUEST_USART5_RX;
    case UartId::Usart6: return DMA_REQUEST_USART6_RX;
#endif
#ifdef HAS_LPUART1
    case UartId::LpUart1: return DMA_REQUEST_LPUART1_RX;
#endif
#ifdef HAS_LPUART2
    case UartId::LpUart2: return DMA_REQUEST_LPUART2_RX;
#endif
    }
    std::unreachable();
  }
}

[[nodiscard]] uint32_t GetDmaRequestId(SpiId         id,
                                       SpiDmaRequest request) noexcept {
  switch (request) {
  case SpiDmaRequest::Tx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_TX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_TX;
#ifdef HAS_SPI3
    case SpiId::Spi3: return DMA_REQUEST_SPI3_TX;
#endif
    }
    std::unreachable();
  case SpiDmaRequest::Rx:
    switch (id) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_RX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_RX;
#ifdef HAS_SPI3
    case SpiId::Spi3: return DMA_REQUEST_SPI3_RX;
#endif
    }
    std::unreachable();
  }
  std::unreachable();
}

[[nodiscard]] uint32_t GetDmaRequestId(TimId         id,
                                       TimDmaRequest request) noexcept {
  switch (id) {
  case TimId::Tim1:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM1_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM1_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM1_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM1_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM1_CH4;
    default: std::unreachable();
    }
#ifdef HAS_TIM2
  case TimId::Tim2:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM2_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM2_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM2_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM2_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM2_CH4;
    default: std::unreachable();
    }
#endif
  case TimId::Tim3:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM3_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM3_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM3_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM3_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM3_CH4;
    default: std::unreachable();
    }
#ifdef HAS_TIM4
  case TimId::Tim4:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM4_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM4_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM4_CH2;
    case TimDmaRequest::Ch3: return DMA_REQUEST_TIM4_CH3;
    case TimDmaRequest::Ch4: return DMA_REQUEST_TIM4_CH4;
    default: std::unreachable();
    }
#endif
#ifdef HAS_TIM15
  case TimId::Tim15:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM15_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM15_CH1;
    case TimDmaRequest::Ch2: return DMA_REQUEST_TIM15_CH2;
    default: std::unreachable();
    }
#endif
  case TimId::Tim16:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM16_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM16_CH1;
    default: std::unreachable();
    }
  case TimId::Tim17:
    switch (request) {
    case TimDmaRequest::PeriodElapsed: return DMA_REQUEST_TIM17_UP;
    case TimDmaRequest::Ch1: return DMA_REQUEST_TIM17_CH1;
    default: std::unreachable();
    }
  default: break;
  }

  std::unreachable();
}

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
  // Enable clocks
  if (n_used_channels > 0) {
    __HAL_RCC_DMA1_CLK_ENABLE();
  }

#ifdef HAS_DMA2
  if (n_used_channels > NDma1Chans) {
    __HAL_RCC_DMA2_CLK_ENABLE();
  }
#endif

  // Enable Interrupts
  if (n_used_channels > 0) {
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  }
  if (n_used_channels > 1) {
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
  }
  if (n_used_channels > 3) {
#if (N_DMA1_CHANS == 5)
    HAL_NVIC_SetPriority(DMA1_Ch4_5_DMAMUX1_OVR_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Ch4_5_DMAMUX1_OVR_IRQn);
#elif (N_DMA1_CHANS == 7) && !defined(HAS_DMA2)
    HAL_NVIC_SetPriority(DMA1_Ch4_7_DMAMUX1_OVR_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Ch4_7_DMAMUX1_OVR_IRQn);
#elif defined(HAS_DMA2)
    HAL_NVIC_SetPriority(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQn);
#else
#error "Invalid N_DMA1_CHANS value"
#endif
  }
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
  static constexpr auto NMaxDmaChannels = NDma1Chans + NDma2Chans;

  using ChanList = hal::DmaChannels<Channels...>;

 public:
  static auto& instance() {
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
             && ((DmaInst == 1 && Chan <= NDma1Chans)
                 || (DmaInst == 2 && Chan <= NDma2Chans)))
  [[nodiscard]] static consteval bool ChannelInUse() noexcept {
    if constexpr (DmaInst == 1) {
      constexpr auto Idx = Chan - 1;
      return Idx < sizeof...(Channels);
    } else if constexpr (DmaInst == 2) {
      constexpr auto Idx = Chan + NDma1Chans - 1;
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
             && ((DmaInst == 1 && Chan <= NDma1Chans)
                 || (DmaInst == 2 && Chan <= NDma2Chans)))
  /**
   * DMA interrupt handler
   * @tparam ChanIdx DMA channel index
   */
  void HandleInterrupt() noexcept {
    if constexpr (DmaInst == 1) {
      HAL_DMA_IRQHandler(&hdmas[Chan - 1]);
    } else if constexpr (DmaInst == 2) {
      HAL_DMA_IRQHandler(&hdmas[Chan + NDma2Chans - 1]);
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
        DMA1_Channel1, DMA1_Channel2, DMA1_Channel3,
        DMA1_Channel4, DMA1_Channel5,
#if (N_DMA1_CHANS > 5)
        DMA1_Channel6, DMA1_Channel7,
#endif
#if defined(HAS_DMA2)
        DMA2_Channel1, DMA2_Channel2, DMA2_Channel3,
        DMA2_Channel4, DMA2_Channel5,
#endif
    };

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

}   // namespace stm32g0