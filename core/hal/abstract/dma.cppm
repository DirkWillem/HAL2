module;

#include <array>
#include <tuple>
#include <utility>

export module hal.abstract:dma;

import :peripheral;

namespace hal {

export enum class DmaDirection { MemToPeriph, PeriphToMem };

export enum class DmaMode { Normal, Circular };

export enum class DmaDataWidth { Byte, HalfWord, Word };

export enum class DmaPriority { Low, Medium, High, VeryHigh };

export template <typename RId>
concept PeripheralReqId = std::equality_comparable<RId>;

export template <typename Impl>
concept DmaChannelId = requires {
  { Impl::Peripheral } -> PeripheralId;
  { Impl::Request } -> PeripheralReqId;
};

export template <typename Impl>
concept DmaChannel = DmaChannelId<Impl> && requires { Impl::Priority; };

export template <typename Impl>
concept DmaTransferCompleteCallback =
    requires(Impl& impl) { impl.DmaTransferCompleteCallback(); };

struct DummyChannel {
  static constexpr auto Peripheral = 1;
  static constexpr auto Request    = 2;
};

template <typename Lhs, typename Rhs>
  requires DmaChannelId<Lhs> && DmaChannelId<Rhs>
[[nodiscard]] consteval bool DmaChanIdEq() noexcept {
  if constexpr (std::is_same_v<std::decay_t<decltype(Lhs::Peripheral)>,
                               std::decay_t<decltype(Rhs::Peripheral)>>
                && std::is_same_v<std::decay_t<decltype(Lhs::Request)>,
                                  std::decay_t<decltype(Rhs::Request)>>) {
    return Lhs::Peripheral == Rhs::Peripheral && Lhs::Request == Rhs::Request;
  }

  return false;
}

template <std::size_t Idx, DmaChannel... Cs>
struct GetChannelByIndexHelper;

template <std::size_t Idx, DmaChannel Cur, DmaChannel... Rest>
struct GetChannelByIndexHelper<Idx, Cur, Rest...> {
  using Channel = GetChannelByIndexHelper<Idx - 1, Rest...>::Channel;
};

template <DmaChannel Cur, DmaChannel... Rest>
struct GetChannelByIndexHelper<0, Cur, Rest...> {
  using Channel = Cur;
};

export template <DmaChannel... Channels>
struct DmaChannels {
  static constexpr std::size_t count = sizeof...(Channels);

  template <hal::DmaChannelId Chan>
  static consteval bool ContainsChanId() noexcept {
    return (... || DmaChanIdEq<Chan, Channels>());
  }

  template <hal::DmaChannelId Chan>
    requires(ContainsChanId<Chan>())
  [[nodiscard]] static consteval std::size_t DmaChannelIndex() noexcept {
    constexpr std::array<bool, sizeof...(Channels)> is_chan{
        DmaChanIdEq<Chan, Channels>()...};

    for (std::size_t i = 0; i < sizeof...(Channels); i++) {
      if (is_chan[i]) {
        return i;
      }
    }

    std::unreachable();
  }

  template <hal::DmaChannelId Chan>
    requires(ContainsChanId<Chan>())
  [[nodiscard]] static consteval DmaPriority DmaChannelPriority() noexcept {
    constexpr std::array<std::tuple<bool, DmaPriority>, sizeof...(Channels)>
        chan_prios{
            std::make_pair(DmaChanIdEq<Chan, Channels>(), Chan::Priority)...};

    for (const auto [is_req_chan, chan_prio] : chan_prios) {
      if (is_req_chan) {
        return chan_prio;
      }
    }

    std::unreachable();
  }

  template <std::size_t Idx>
    requires(Idx < sizeof...(Channels))
  [[nodiscard]] static consteval auto GetPeripheralByIndex() noexcept {
    return GetChannelByIndexHelper<Idx, Channels...>::Channel::Peripheral;
  }
};

export template <typename Impl>
concept Dma = requires(Impl impl) {
  {
    Impl::template ChannelEnabled<DummyChannel>()
  } -> std::convertible_to<bool>;

  impl.template SetupChannel<DummyChannel>(
      std::declval<DmaDirection>(), std::declval<DmaMode>(),
      std::declval<DmaDataWidth>(), std::declval<bool>(),
      std::declval<DmaDataWidth>(), std::declval<bool>());
};

}   // namespace hal