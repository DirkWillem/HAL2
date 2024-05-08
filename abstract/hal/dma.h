#pragma once

#include <array>
#include <tuple>
#include <utility>

#include "peripheral.h"

namespace hal {

enum class DmaDirection { MemToPeriph, PeriphToMem };

enum class DmaMode { Normal, Circular };

enum class DmaDataWidth { Byte, HalfWord, Word };

enum class DmaPriority { Low, Medium, High, VeryHigh };

template <typename RId>
concept PeripheralReqId = std::equality_comparable<RId>;

template <typename Impl>
concept DmaChannelId = requires {
  { Impl::Peripheral } -> PeripheralId;
  { Impl::Request } -> PeripheralReqId;
};

template <typename Impl>
concept DmaChannel = DmaChannelId<Impl> && requires { Impl::Priority; };

namespace detail {

struct DummyChannel {
  static constexpr auto Peripheral = 1;
  static constexpr auto Request    = 2;
};

template <hal::DmaChannelId Lhs, hal::DmaChannelId Rhs>
[[nodiscard]] static consteval bool DmaChanIdEq() noexcept {
  if constexpr (std::is_same_v<std::decay_t<decltype(Lhs::Peripheral)>,
                               std::decay_t<decltype(Rhs::Peripheral)>>
                && std::is_same_v<std::decay_t<decltype(Lhs::Request)>,
                                  std::decay_t<decltype(Rhs::Request)>>) {
    return Lhs::Peripheral == Rhs::Peripheral && Lhs::Request == Rhs::Request;
  } else {
    return false;
  }
}

}   // namespace detail

template <DmaChannel... Channels>
struct DmaChannels {
  template <hal::DmaChannelId Chan>
  static consteval bool ContainsChanId() noexcept {
    return (... || detail::DmaChanIdEq<Chan, Channels>());
  }

  template <hal::DmaChannelId Chan>
    requires(ContainsChanId<Chan>())
  [[nodiscard]] static consteval std::size_t DmaChannelIndex() noexcept {
    constexpr std::array<bool, sizeof...(Channels)> is_chan{
        detail::DmaChanIdEq<Chan, Channels>()...};

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
        chan_prios{std::make_pair(detail::DmaChanIdEq<Chan, Channels>(),
                                  Chan::Priority)...};

    for (const auto [is_req_chan, chan_prio] : chan_prios) {
      if (is_req_chan) {
        return chan_prio;
      }
    }

    std::unreachable();
  }
};

template <typename Impl>
concept Dma = requires(Impl impl) {
  {
    Impl::template ChannelEnabled<detail::DummyChannel>()
  } -> std::convertible_to<bool>;

  impl.template SetupChannel<detail::DummyChannel>(
      std::declval<DmaDirection>(), std::declval<DmaMode>(),
      std::declval<DmaDataWidth>(), std::declval<bool>(),
      std::declval<DmaDataWidth>(), std::declval<bool>());
};

}   // namespace hal