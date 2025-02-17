#pragma once

#include <halstd/callback.h>
#include <halstd/mp/values.h>

#include <hal/peripheral.h>

#include "pin.h"

namespace stm32g0 {

struct InterruptPinConfig {
  PinId        pin;
  Edge         edge;
  hal::PinPull pull = hal::PinPull::NoPull;
};

namespace detail {

void EnableExtiInterrupt(PinId pin) noexcept;

}

template <typename Impl, InterruptPinConfig... Pins>
class PinInterruptImpl : public hal::UsedPeripheral {
 public:
  static Impl& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  PinInterruptImpl() noexcept
      : hal::UsedPeripheral{} {
    (..., ([](InterruptPinConfig cfg) {
       Pin::InitializeInterrupt(cfg.pin, cfg.edge, cfg.pull);
       detail::EnableExtiInterrupt(cfg.pin);
     })(Pins));
  }

  [[nodiscard]] static consteval bool PinInterruptActive(unsigned pin_num,
                                                         Edge edge) noexcept {
    return (... || ([pin_num, edge](InterruptPinConfig cfg) {
              return cfg.pin.num == pin_num
                     && (cfg.edge == edge || cfg.edge == Edge::Both);
            })(Pins));
  }

  template <PinId Pin>
  constexpr void RegisterCallback(halstd::Callback<>& callback) noexcept {
    callbacks[GetIndex<Pin.num>()] = &callback;
  }

  template <unsigned Pin, Edge Edge>
  constexpr void HandleInterrupt() noexcept {
    constexpr auto Idx = GetIndex<Pin>();
    if (callbacks[Idx] != nullptr) {
      (*callbacks[Idx])();
    }
  }

 private:
  template <unsigned Pin>
  consteval static std::size_t GetIndex() noexcept {
    using Ps = halstd::Values<InterruptPinConfig, Pins...>;
    return Ps::GetIndexBy([](auto cfg) { return cfg.pin.num == Pin; });
  }

  std::array<halstd::Callback<>*, sizeof...(Pins)> callbacks{};
};

struct PinInterruptImplMarker {};

template <typename M>
class PinInterrupt : public hal::UnusedPeripheral<PinInterrupt<M>> {
 public:
  [[nodiscard]] static consteval bool PinInterruptActive(unsigned,
                                                         Edge) noexcept {
    std::unreachable();
    return false;
  }

  template <unsigned, Edge>
  constexpr void HandleInterrupt() noexcept {
    std::unreachable();
  }
};

}   // namespace stm32g0