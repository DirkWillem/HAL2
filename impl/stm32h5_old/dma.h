#pragma once

#include <array>
#include <utility>

#include <hal/dma.h>

#include <stm32h5xx_hal.h>

#include <stm32h5_old/peripheral_ids.h>

namespace stm32h5 {

template <hal::DmaChannel... Channels>
/**
 * Type alias for a DMA channel list
 * @tparam Channels DMA channels
 */
using DmaChannels = hal::DmaChannels<Channels...>;

/** Possible DMA requests for UART */
enum class UartDmaRequest { Tx, Rx };

/** Possible DMA requests for I2C */
enum class I2cDmaRequest { Tx, Rx };

/** Possible DMA requests for SPI */
enum class SpiDmaRequest { Tx, Rx };

namespace detail {

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

[[nodiscard]] uint32_t ToHalDmaDirection(hal::DmaDirection dir) noexcept;
[[nodiscard]] uint32_t ToHalDmaMode(hal::DmaMode mode) noexcept;
[[nodiscard]] uint32_t ToHalSrcDataWidth(hal::DmaDataWidth data_width) noexcept;
[[nodiscard]] uint32_t ToHalDstDataWidth(hal::DmaDataWidth data_width) noexcept;

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

void SetupDma(std::size_t n_used_channels) noexcept;

}   // namespace detail

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

template <auto Periph, auto Req, hal::DmaPriority Prio = hal::DmaPriority::Low>
/**
 * DMA channel definition
 * @tparam Periph Channel peripheral
 * @tparam Req Channel request type
 * @tparam Prio Channel priority
 */
struct DmaChannel {
  static constexpr auto Peripheral = Periph;
  static constexpr auto Request    = Req;
  static constexpr auto Priority   = Prio;
};

template <typename Impl, typename Channels>
class DmaImpl;

template <typename Impl, hal::DmaChannel... Channels>
class DmaImpl<Impl, hal::DmaChannels<Channels...>>
    : public hal::UsedPeripheral {
#if defined(STM32H533xx)
  static constexpr auto NGpDma1Channels = 8u;
  static constexpr auto NGpDma2Channels = 8u;
#else
#error "Cannot determine number of DMA channels for this STM32H5 variant"
#endif

  static constexpr auto NMaxDmaChannels = NGpDma1Channels + NGpDma2Channels;

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
        .Request   = detail::GetDmaRequestId(Chan::Peripheral, Chan::Request),
        .Direction = detail::ToHalDmaDirection(dir),
        .SrcInc    = src_inc ? DMA_SINC_INCREMENTED : DMA_SINC_FIXED,
        .DestInc   = dst_inc ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED,
        .SrcDataWidth  = detail::ToHalSrcDataWidth(src_width),
        .DestDataWidth = detail::ToHalDstDataWidth(dst_width),
        .Priority      = detail::ToHalDmaPriority(
            ChanList::template DmaChannelPriority<Chan>()),
        .SrcBurstLength  = 1,
        .DestBurstLength = 1,
        .Mode            = detail::ToHalDmaMode(mode),
    };
    HAL_DMA_Init(&hdma);

    return hdma;
  }

  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NGpDma1Channels)
             || (DmaInst == 2 && Chan < NGpDma2Channels))
  /**
   * DMA interrupt handler
   * @tparam ChanIdx DMA channel index
   */
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
  DmaImpl() { detail::SetupDma(sizeof...(Channels)); }

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

struct DmaImplMarker {};

template <typename M>
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