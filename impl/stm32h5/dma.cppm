module;

#include <utility>

#include <stm32h5xx_hal.h>

export module hal.stm32h5:dma;

import hal.abstract;

import :nvic;
import :peripherals;

namespace stm32h5 {

/**
 * Type alias for a DMA channel list.
 * @tparam Channels DMA channels.
 */
export template <hal::DmaChannel... Channels>
using DmaChannels = hal::DmaChannels<Channels...>;

/** Possible DMA requests for UART. */
export enum class UartDmaRequest {
  Tx,   //!< UART transmit
  Rx,   //!< UART receive
};

/** Possible DMA requests for I2C. */
export enum class I2cDmaRequest {
  Tx,   //!< I2C transmit
  Rx,   //!< I2C receive
};

/** Possible DMA requests for SPI. */
export enum class SpiDmaRequest {
  Tx,   //!< SPI transmit
  Rx,   //!< SPI receive
};

[[nodiscard]] constexpr uint32_t
GetDmaRequestId(UartId uart, UartDmaRequest request) noexcept {
  switch (request) {
  case UartDmaRequest::Tx:
    switch (uart) {
    case UartId::Usart1: return GPDMA1_REQUEST_USART1_TX;
    case UartId::Usart2: return GPDMA1_REQUEST_USART2_TX;
    case UartId::Usart3: return GPDMA1_REQUEST_USART3_TX;
    case UartId::Uart4: return GPDMA1_REQUEST_UART4_TX;
    case UartId::Uart5: return GPDMA1_REQUEST_UART5_TX;
    case UartId::Usart6: return GPDMA1_REQUEST_USART6_TX;
    case UartId::LpUart1: return GPDMA1_REQUEST_USART1_TX;
    }

    std::unreachable();
  case UartDmaRequest::Rx:
    switch (uart) {
    case UartId::Usart1: return GPDMA1_REQUEST_USART1_RX;
    case UartId::Usart2: return GPDMA1_REQUEST_USART2_RX;
    case UartId::Usart3: return GPDMA1_REQUEST_USART3_RX;
    case UartId::Uart4: return GPDMA1_REQUEST_UART4_RX;
    case UartId::Uart5: return GPDMA1_REQUEST_UART5_RX;
    case UartId::Usart6: return GPDMA1_REQUEST_USART6_RX;
    case UartId::LpUart1: return GPDMA1_REQUEST_USART1_RX;
    }

    std::unreachable();
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t
GetDmaRequestId(SpiId spi, SpiDmaRequest request) noexcept {
  switch (request) {
  case SpiDmaRequest::Tx:
    switch (spi) {
    case SpiId::Spi1: return GPDMA1_REQUEST_SPI1_TX;
    case SpiId::Spi2: return GPDMA1_REQUEST_SPI2_TX;
    case SpiId::Spi3: return GPDMA1_REQUEST_SPI3_TX;
    case SpiId::Spi4: return GPDMA1_REQUEST_SPI4_TX;
    }

    std::unreachable();
  case SpiDmaRequest::Rx:
    switch (spi) {
    case SpiId::Spi1: return GPDMA1_REQUEST_SPI1_RX;
    case SpiId::Spi2: return GPDMA1_REQUEST_SPI2_RX;
    case SpiId::Spi3: return GPDMA1_REQUEST_SPI3_RX;
    case SpiId::Spi4: return GPDMA1_REQUEST_SPI4_RX;
    }

    std::unreachable();
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
  case hal::DmaMode::Circular: return DMA_LINKEDLIST_CIRCULAR;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalSrcDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_SRC_DATAWIDTH_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_SRC_DATAWIDTH_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_SRC_DATAWIDTH_WORD;
  }

  std::unreachable();
}

[[nodiscard]] uint32_t
ToHalDstDataWidth(hal::DmaDataWidth data_width) noexcept {
  switch (data_width) {
  case hal::DmaDataWidth::Byte: return DMA_DEST_DATAWIDTH_BYTE;
  case hal::DmaDataWidth::HalfWord: return DMA_DEST_DATAWIDTH_HALFWORD;
  case hal::DmaDataWidth::Word: return DMA_DEST_DATAWIDTH_WORD;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t
ToHalDmaPriority(hal::DmaPriority prio) noexcept {
  switch (prio) {
  case hal::DmaPriority::Low: return DMA_LOW_PRIORITY_LOW_WEIGHT;
  case hal::DmaPriority::Medium: return DMA_LOW_PRIORITY_MID_WEIGHT;
  case hal::DmaPriority::High: return DMA_LOW_PRIORITY_HIGH_WEIGHT;
  case hal::DmaPriority::VeryHigh: return DMA_HIGH_PRIORITY;
  }

  std::unreachable();
}

template <typename Impl>
void SetupDma(std::size_t n_used_channels) noexcept {
#if defined(STM32H533xx)
  // Enable clocks
  if (n_used_channels > 0) {
    __HAL_RCC_GPDMA1_CLK_ENABLE();
  }

  if (n_used_channels > 8) {
    __HAL_RCC_GPDMA2_CLK_ENABLE();
  }

  // Enable interrupts for all enabled channels
  constexpr std::array<IRQn_Type, 16> ChannelIrqns{
      GPDMA1_Channel0_IRQn, GPDMA1_Channel1_IRQn, GPDMA1_Channel2_IRQn,
      GPDMA1_Channel3_IRQn, GPDMA1_Channel4_IRQn, GPDMA1_Channel5_IRQn,
      GPDMA1_Channel6_IRQn, GPDMA1_Channel7_IRQn, GPDMA2_Channel0_IRQn,
      GPDMA2_Channel1_IRQn, GPDMA2_Channel2_IRQn, GPDMA2_Channel3_IRQn,
      GPDMA2_Channel4_IRQn, GPDMA2_Channel5_IRQn, GPDMA2_Channel6_IRQn,
      GPDMA2_Channel7_IRQn,
  };

  [ChannelIrqns]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
    (..., EnableInterrupt<ChannelIrqns[Idxs], Impl>());
  }(std::make_index_sequence<ChannelIrqns.size()>());
#else
#error "Cannot determine DMA interrupts for this STM32H5 variant"
#endif
}

template <auto Periph, auto Req>
/**
 * DMA channel ID
 * @tparam Periph Channel peripheral
 * @tparam Req Channel request type
 */
struct DmaChannelId {
  static constexpr auto Peripheral = Periph;
  static constexpr auto Request    = Req;
};

/**
 * DMA channel definition.
 * @tparam Periph Channel peripheral.
 * @tparam Req Channel request type.
 * @tparam Prio Channel priority.
 */
template <auto Periph, auto Req, hal::DmaPriority Prio = hal::DmaPriority::Low>
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
#if defined(STM32H533xx)
  static constexpr auto NGpDma1Channels = 8u;
  static constexpr auto NGpDma2Channels = 8u;
#else
#error "Cannot determine number of DMA channels for this STM32H5 variant."
#endif

  static constexpr auto NMaxDmaChannels = NGpDma1Channels + NGpDma2Channels;

  using ChanList = hal::DmaChannels<Channels...>;

 public:
  /**
   * @brief Returns a singleton instance of the DMA.
   *
   * @return Singleton instance of the inheriting \c Impl class.
   */
  static Impl& instance() {
    static Impl inst{};
    return inst;
  }

  /**
   * @brief Returns whether the given channel is enabled.
   *
   * @tparam Chan Channel to check.
   * @return Whether Chan is enabled.
   */
  template <hal::DmaChannelId Chan>
  [[nodiscard]] static constexpr bool ChannelEnabled() noexcept {
    return ChanList::template ContainsChanId<Chan>();
  }

  /**
   * @brief Returns whether a DMA channel is in use.
   *
   * @tparam DmaInst DMA instance number (either \c 1 or \c 2).
   * @tparam Chan DMA channel number.
   * @return Whether the given DMA channel is in use.
   */
  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NGpDma1Channels)
             || (DmaInst == 2 && Chan < NGpDma2Channels))
  [[nodiscard]] static consteval bool ChannelInUse() noexcept {
    if constexpr (DmaInst == 1) {
      constexpr auto Idx = Chan;
      return Idx < sizeof...(Channels);
    } else if constexpr (DmaInst == 2) {
      constexpr auto Idx = Chan + NGpDma1Channels;
      return Idx < sizeof...(Channels);
    } else {
      std::unreachable();
    }
  }

  /**
   * @brief Sets up a DMA channel.
   *
   * @tparam Chan Channel to set up.
   * @param dir Channel direction.
   * @param mode Channel mode.
   * @param periph_data_width Peripheral data width.
   * @param mem_data_width Memory data width.
   * @return Reference to the DMA handle.
   */
  template <hal::DmaChannelId Chan>
  [[nodiscard]] DMA_HandleTypeDef&
  SetupChannel(hal::DmaDirection dir, hal::DmaMode mode,
               hal::DmaDataWidth periph_data_width, bool periph_inc,
               hal::DmaDataWidth mem_data_width, bool mem_inc) noexcept {
    constexpr auto     Idx  = ChanList::template DmaChannelIndex<Chan>();
    DMA_HandleTypeDef& hdma = hdmas[Idx];

    hdma.Instance = DmaChannel<Chan>();
    const auto src_inc =
        (dir == hal::DmaDirection::MemToPeriph && mem_inc)
        || (dir == hal::DmaDirection::PeriphToMem && periph_inc);
    const auto dst_inc = (dir == hal::DmaDirection::MemToPeriph && periph_inc)
                         || (dir == hal::DmaDirection::PeriphToMem && mem_inc);

    const auto src_width = dir == hal::DmaDirection::MemToPeriph
                               ? mem_data_width
                               : periph_data_width;
    const auto dst_width = dir == hal::DmaDirection::MemToPeriph
                               ? periph_data_width
                               : mem_data_width;

    hdma.Init = {
        .Request       = GetDmaRequestId(Chan::Peripheral, Chan::Request),
        .BlkHWRequest  = DMA_BREQ_SINGLE_BURST,
        .Direction     = ToHalDmaDirection(dir),
        .SrcInc        = src_inc ? DMA_SINC_INCREMENTED : DMA_SINC_FIXED,
        .DestInc       = dst_inc ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED,
        .SrcDataWidth  = ToHalSrcDataWidth(src_width),
        .DestDataWidth = ToHalDstDataWidth(dst_width),
        .Priority =
            ToHalDmaPriority(ChanList::template DmaChannelPriority<Chan>()),
        .SrcBurstLength        = 1,
        .DestBurstLength       = 1,
        .TransferAllocatedPort = 0,
        .TransferEventMode     = 0,
        .Mode                  = ToHalDmaMode(mode),
    };
    HAL_DMA_Init(&hdma);

    return hdma;
  }

  /**
   * @brief DMA interrupt handler.
   *
   * @tparam DmaInst DMA instance number.
   * @tparam Chan DMA channel number.
   */
  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NGpDma1Channels)
             || (DmaInst == 2 && Chan < NGpDma2Channels))
  void HandleInterrupt() noexcept {
    if constexpr (DmaInst == 1) {
      HAL_DMA_IRQHandler(&hdmas[Chan]);
    } else if constexpr (DmaInst == 2) {
      HAL_DMA_IRQHandler(&hdmas[Chan + NGpDma1Channels]);
    } else {
      std::unreachable();
    }
  }

 private:
  DmaImpl() { SetupDma<Impl>(sizeof...(Channels)); }

  template <hal::DmaChannelId Chan>
    requires(ChanList::template ContainsChanId<Chan>())
  [[nodiscard]] static constexpr DMA_Channel_TypeDef* DmaChannel() noexcept {
    const std::array<DMA_Channel_TypeDef*, NMaxDmaChannels> channels{
        GPDMA1_Channel0, GPDMA1_Channel1, GPDMA1_Channel2, GPDMA1_Channel3,
        GPDMA1_Channel4, GPDMA1_Channel5, GPDMA1_Channel6, GPDMA1_Channel7,
        GPDMA2_Channel0, GPDMA2_Channel1, GPDMA2_Channel2, GPDMA2_Channel3,
        GPDMA2_Channel4, GPDMA2_Channel5, GPDMA2_Channel6, GPDMA2_Channel7};

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

}   // namespace stm32h5