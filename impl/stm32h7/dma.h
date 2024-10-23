#pragma once

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

#include <hal/dma.h>

#include <stm32h7xx_hal.h>

#include <stm32h7/peripheral_ids.h>

#include <constexpr_tools/type_helpers.h>

#include "core.h"

namespace stm32h7 {

template <hal::DmaChannel... Chans>
using DmaChannels = hal::DmaChannels<Chans...>;

/** Possible DMA requests for SPI */
enum class SpiDmaRequest { Tx, Rx };

namespace detail {

[[nodiscard]] consteval bool IsBdmaPeripheral(SpiId spi) noexcept {
  return spi == SpiId::Spi6;
}

template <hal::DmaChannel Chan>
using IsBdmaChannel_t =
    std::bool_constant<IsBdmaPeripheral(std::get<1>(Chan::Peripheral))>;

template <hal::DmaChannel Chan>
inline constexpr auto IsBdmaChannel = IsBdmaChannel_t<Chan>::value;

template <hal::DmaChannel Chan>
inline constexpr auto IsChannelForCurrentCore =
    std::get<0>(Chan::Peripheral) == CurrentCore;

[[nodiscard]] constexpr uint32_t
GetDmaRequestId(SpiId spi, SpiDmaRequest request) noexcept {
  switch (request) {
  case SpiDmaRequest::Tx:
    switch (spi) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_TX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_TX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_TX;
    case SpiId::Spi4: return DMA_REQUEST_SPI4_TX;
    case SpiId::Spi5: return DMA_REQUEST_SPI5_TX;
    case SpiId::Spi6: return BDMA_REQUEST_SPI6_TX;
    }

    std::unreachable();
  case SpiDmaRequest::Rx:
    switch (spi) {
    case SpiId::Spi1: return DMA_REQUEST_SPI1_RX;
    case SpiId::Spi2: return DMA_REQUEST_SPI2_RX;
    case SpiId::Spi3: return DMA_REQUEST_SPI3_RX;
    case SpiId::Spi4: return DMA_REQUEST_SPI4_RX;
    case SpiId::Spi5: return DMA_REQUEST_SPI5_RX;
    case SpiId::Spi6: return BDMA_REQUEST_SPI6_RX;
    }

    std::unreachable();
  }
}

[[nodiscard]] uint32_t ToHalDmaDirection(hal::DmaDirection dir) noexcept;
[[nodiscard]] uint32_t ToHalDmaMode(hal::DmaMode mode) noexcept;
[[nodiscard]] uint32_t ToHalMemDataWidth(hal::DmaDataWidth data_width) noexcept;
[[nodiscard]] uint32_t
ToHalPeriphDataWidth(hal::DmaDataWidth data_width) noexcept;
[[nodiscard]] constexpr uint32_t
ToHalDmaPriority(hal::DmaPriority prio) noexcept {
  switch (prio) {
  case hal::DmaPriority::Low: return DMA_PRIORITY_LOW;
  case hal::DmaPriority::Medium: return DMA_PRIORITY_MEDIUM;
  case hal::DmaPriority::High: return DMA_PRIORITY_HIGH;
  case hal::DmaPriority::VeryHigh: return DMA_PRIORITY_VERY_HIGH;
  }

  std::unreachable();
}

}   // namespace detail

template <Core C, auto Periph, auto Req>
/**
 * DMA channel ID
 * @tparam Periph Channel peripheral
 * @tparam Req Channel request type
 */
struct DmaChannelId {
  static constexpr auto Peripheral = std::make_pair(C, Periph);
  static constexpr auto Request    = Req;
};

template <Core C, auto Periph, auto Req,
          hal::DmaPriority Prio = hal::DmaPriority::Low>
/**
 * DMA channel definition
 * @tparam Periph Channel peripheral
 * @tparam Req Channel request type
 * @tparam Prio Channel priority
 */
struct DmaChannel {
  static constexpr auto Peripheral = std::make_pair(C, Periph);
  static constexpr auto Request    = Req;
  static constexpr auto Priority   = Prio;
};

template <typename Impl, typename... Channels>
class DmaImpl;

template <typename Impl, hal::DmaChannel... Chans>
class DmaImpl<Impl, DmaChannels<Chans...>> : public hal::UsedPeripheral {
#if defined(STM32H755)
  static constexpr auto NDma1Channels = 8u;
  static constexpr auto NDma2Channels = 8u;
  static constexpr auto NBdmaChannels = 8u;
#else
#error "Cannot determine number of DMA channels for this STM32H7 variant"
#endif

  static constexpr auto NMaxDmaChannels  = NDma1Channels + NDma2Channels;
  static constexpr auto NMaxBdmaChannels = NBdmaChannels;

  using DmaBdmaPartitioning =
      ct::PartitionTypes<detail::IsBdmaChannel_t, hal::DmaChannels, Chans...>;

  using AllChannels  = hal::DmaChannels<Chans...>;
  using BdmaChannels = DmaBdmaPartitioning::Matched;
  using DmaChannels  = DmaBdmaPartitioning::Unmatched;

 public:
  static Impl& instance() {
    static Impl inst{};
    return inst;
  }

  template <hal::DmaChannelId Chan>
  [[nodiscard]] static consteval bool ChannelEnabled() noexcept {
    return DmaChannels::template ContainsChanId<Chan>()
           || BdmaChannels::template ContainsChanId<Chan>();
  }

  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NDma1Channels)
                 || (DmaInst == 2 && Chan < NDma2Channels))
  [[nodiscard]] static consteval bool DmaChannelInUseForCurrentCore() noexcept {
    if constexpr (DmaInst == 1) {
      constexpr auto Idx = Chan;

      if constexpr (Idx < DmaChannels::count) {
        constexpr auto Peripheral =
            DmaChannels::template GetPeripheralByIndex<Idx>();
        return std::get<0>(Peripheral) == CurrentCore
               && Idx < DmaChannels::count;
      } else {
        return false;
      }
    } else if constexpr (DmaInst == 2) {
      constexpr auto Idx = Chan + NDma1Channels;
      if constexpr (Idx < DmaChannels::count) {
        constexpr auto Peripheral =
            DmaChannels::template GetPeripheralByIndex<Idx>();
        return std::get<0>(Peripheral) == CurrentCore
               && Idx < DmaChannels::count;
      } else {
        return false;
      }
    } else {
      std::unreachable();
    }
  }

  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NBdmaChannels))
  [[nodiscard]] static consteval bool
  BdmaChannelInUseForCurrentCore() noexcept {
    if constexpr (DmaInst == 1) {
      constexpr auto Idx = Chan;

      if constexpr (Idx < BdmaChannels::count) {
        constexpr auto Peripheral =
            BdmaChannels::template GetPeripheralByIndex<Idx>();
        return std::get<0>(Peripheral) == CurrentCore
               && Idx < BdmaChannels::count;
      } else {
        return false;
      }
    } else {
      std::unreachable();
    }
  }

  template <hal::DmaChannelId Chan>
  [[nodiscard]] DMA_HandleTypeDef&
  SetupChannel(hal::DmaDirection dir, hal::DmaMode mode,
               hal::DmaDataWidth periph_data_width, bool periph_inc,
               hal::DmaDataWidth mem_data_width, bool mem_inc) noexcept {
    constexpr auto Idx = ChannelIndex<Chan>();
    constexpr auto ReqId =
        detail::GetDmaRequestId(std::get<1>(Chan::Peripheral), Chan::Request);
    auto& hdma = detail::IsBdmaChannel<Chan> ? hbdmas[Idx] : hdmas[Idx];

    hdma.Instance = DmaChannel<Chan>();
    hdma.Init     = {
            .Request             = ReqId,
            .Direction           = detail::ToHalDmaDirection(dir),
            .PeriphInc           = periph_inc ? DMA_PINC_ENABLE : DMA_PINC_DISABLE,
            .MemInc              = mem_inc ? DMA_MINC_ENABLE : DMA_MINC_DISABLE,
            .PeriphDataAlignment = detail::ToHalPeriphDataWidth(periph_data_width),
            .MemDataAlignment    = detail::ToHalMemDataWidth(mem_data_width),
            .Mode                = detail::ToHalDmaMode(mode),
            .Priority            = detail::ToHalDmaPriority(
            AllChannels::template DmaChannelPriority<Chan>()),
    };

    HAL_DMA_Init(&hdma);

    return hdma;
  }

  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NDma1Channels)
                 || (DmaInst == 2 && Chan < NDma2Channels))
  /**
   * DMA interrupt handler
   * @tparam ChanIdx DMA channel index
   */
  void HandleDmaInterrupt() noexcept {
    if constexpr (DmaInst == 1) {
      HAL_DMA_IRQHandler(&hdmas[Chan]);
    } else if constexpr (DmaInst == 2) {
      HAL_DMA_IRQHandler(&hdmas[Chan + NDma1Channels]);
    } else {
      std::unreachable();
    }
  }

  template <unsigned DmaInst, unsigned Chan>
    requires((DmaInst == 1 && Chan < NDma1Channels)
                 || (DmaInst == 2 && Chan < NDma2Channels))
  /**
   * BDMA interrupt handler
   * @tparam ChanIdx BDMA channel index
   */
  void HandleBdmaInterrupt() noexcept {
    if constexpr (DmaInst == 1) {
      HAL_DMA_IRQHandler(&hbdmas[Chan]);
    } else {
      std::unreachable();
    }
  }

 protected:
  DmaImpl() noexcept {
    // Enable clocks
    if constexpr (BdmaChannels::count > 0) {
      __HAL_RCC_BDMA_CLK_ENABLE();
    }

    if constexpr (DmaChannels::count > 0) {
      __HAL_RCC_DMA1_CLK_ENABLE();
    }

    if constexpr (DmaChannels::count > NDma1Channels) {
      __HAL_RCC_DMA2_CLK_ENABLE();
    }

    // Enable BDMA interrupts
    static constexpr std::array<IRQn_Type, NBdmaChannels> BdmaChannelIrqns{
        BDMA_Channel0_IRQn, BDMA_Channel1_IRQn, BDMA_Channel2_IRQn,
        BDMA_Channel3_IRQn, BDMA_Channel4_IRQn, BDMA_Channel5_IRQn,
        BDMA_Channel6_IRQn, BDMA_Channel7_IRQn,
    };

    for (std::size_t i = 0; i < BdmaChannels::count; i++) {
      HAL_NVIC_SetPriority(BdmaChannelIrqns[i], 0, 0);
      HAL_NVIC_EnableIRQ(BdmaChannelIrqns[i]);
    }

    // Enable DMA1/2 interrupts

    static constexpr std::array<IRQn_Type, NDma1Channels + NDma2Channels>
        DmaChannelIrqns{
            DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn,
            DMA1_Stream3_IRQn, DMA1_Stream4_IRQn, DMA1_Stream5_IRQn,
            DMA1_Stream6_IRQn, DMA1_Stream7_IRQn, DMA2_Stream0_IRQn,
            DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn,
            DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn,
            DMA2_Stream7_IRQn,
        };

    for (std::size_t i = 0; i < DmaChannels::count; i++) {
      HAL_NVIC_SetPriority(DmaChannelIrqns[i], 0, 0);
      HAL_NVIC_EnableIRQ(DmaChannelIrqns[i]);
    }
  }

 private:
  template <hal::DmaChannelId Chan>
    requires(DmaChannels::template ContainsChanId<Chan>())
  [[nodiscard]] static constexpr DMA_Stream_TypeDef* DmaChannel() noexcept {
    const std::array<DMA_Stream_TypeDef*, NMaxDmaChannels> streams{
        DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3,
        DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7,
        DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3,
        DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7};
    return streams[DmaChannels::template DmaChannelIndex<Chan>()];
  }

  template <hal::DmaChannelId Chan>
    requires(BdmaChannels::template ContainsChanId<Chan>())
  [[nodiscard]] static constexpr BDMA_Channel_TypeDef* DmaChannel() noexcept {
    const std::array<BDMA_Channel_TypeDef*, NMaxBdmaChannels> chans{
        BDMA_Channel0, BDMA_Channel1, BDMA_Channel2, BDMA_Channel3,
        BDMA_Channel4, BDMA_Channel5, BDMA_Channel6, BDMA_Channel7};
    return chans[BdmaChannels::template DmaChannelIndex<Chan>()];
  }

  template <hal::DmaChannelId Chan>
  [[nodiscard]] static consteval std::size_t ChannelIndex() noexcept {
    if constexpr (detail::IsBdmaChannel<Chan>) {
      return BdmaChannels::template DmaChannelIndex<Chan>();
    } else {
      return DmaChannels::template DmaChannelIndex<Chan>();
    }
  }

  std::array<DMA_HandleTypeDef, DmaChannels::count>  hdmas{};
  std::array<DMA_HandleTypeDef, BdmaChannels::count> hbdmas{};
};

struct DmaImplMarker {};

template <typename M>
class Dma : public hal::UnusedPeripheral<Dma<M>> {
 public:
  template <unsigned DmaInst, unsigned Chan>
  [[nodiscard]] static constexpr bool DmaChannelInUseForCurrentCore() noexcept {
    std::unreachable();
    return false;
  }

  template <unsigned DmaInst, unsigned Chan>
  [[nodiscard]] static constexpr bool
  BdmaChannelInUseForCurrentCore() noexcept {
    std::unreachable();
    return false;
  }

  template <unsigned DmaInst, unsigned Chan>
  constexpr void HandleDmaInterrupt() noexcept {
    std::unreachable();
  }

  template <unsigned DmaInst, unsigned Chan>
  constexpr void HandleBdmaInterrupt() noexcept {
    std::unreachable();
  }
};

}   // namespace stm32h7