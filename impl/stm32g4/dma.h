#pragma once

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

#include <hal/dma.h>

#include <stm32g4/peripheral_ids.h>

namespace stm32g4 {

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

[[nodiscard]] uint32_t GetDmaRequestId(UartId         id,
                                       UartDmaRequest request) noexcept;
[[nodiscard]] uint32_t GetDmaRequestId(I2cId         id,
                                       I2cDmaRequest request) noexcept;
[[nodiscard]] uint32_t GetDmaRequestId(SpiId         id,
                                       SpiDmaRequest request) noexcept;

[[nodiscard]] uint32_t ToHalDmaDirection(hal::DmaDirection dir) noexcept;
[[nodiscard]] uint32_t ToHalDmaMode(hal::DmaMode mode) noexcept;
[[nodiscard]] uint32_t ToHalMemDataWidth(hal::DmaDataWidth data_width) noexcept;
[[nodiscard]] uint32_t
ToHalPeriphDataWidth(hal::DmaDataWidth data_width) noexcept;
[[nodiscard]] uint32_t ToHalDmaPriority(hal::DmaPriority prio);

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
            .Request   = detail::GetDmaRequestId(Chan::Peripheral, Chan::Request),
            .Direction = detail::ToHalDmaDirection(dir),
            .PeriphInc = periph_inc ? DMA_PINC_ENABLE : DMA_PINC_DISABLE,
            .MemInc    = mem_inc ? DMA_MINC_ENABLE : DMA_MINC_DISABLE,
            .PeriphDataAlignment = detail::ToHalPeriphDataWidth(periph_data_width),
            .MemDataAlignment    = detail::ToHalMemDataWidth(mem_data_width),
            .Mode                = detail::ToHalDmaMode(mode),
            .Priority            = detail::ToHalDmaPriority(
            ChanList::template DmaChannelPriority<Chan>()),
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
  DmaImpl() { detail::SetupDma(sizeof...(Channels)); }

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

}   // namespace stm32g4