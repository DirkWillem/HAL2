module;

#include <array>
#include <utility>

#include <stm32g0xx_hal.h>

export module hal.stm32g0:pin_interrupt;

import hstd;

import hal.abstract;

import :pin;

namespace stm32g0 {

export struct InterruptPinConfig {
  PinId        pin;
  hal::Edge    edge;
  hal::PinPull pull = hal::PinPull::NoPull;
};

[[nodiscard]] constexpr IRQn_Type GetExtiIrqNumber(PinNum pin_num) noexcept {
  switch (pin_num) {
  case 0: [[fallthrough]];
  case 1: return EXTI0_1_IRQn;
  case 2: [[fallthrough]];
  case 3: return EXTI2_3_IRQn;
  case 4: [[fallthrough]];
  case 5: [[fallthrough]];
  case 6: [[fallthrough]];
  case 7: [[fallthrough]];
  case 8: [[fallthrough]];
  case 9: [[fallthrough]];
  case 10: [[fallthrough]];
  case 11: [[fallthrough]];
  case 12: [[fallthrough]];
  case 13: [[fallthrough]];
  case 14: [[fallthrough]];
  case 15: return EXTI4_15_IRQn;
  default: std::unreachable();
  }
}

void EnableExtiInterrupt(PinId pin) noexcept {
  const auto irqn = GetExtiIrqNumber(pin.num);

  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);
}

export template <typename Impl, InterruptPinConfig... Pins>
class PinInterruptImpl : public hal::UsedPeripheral {
 public:
  using PinType = PinId;

  static Impl& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  PinInterruptImpl() noexcept
      : hal::UsedPeripheral{} {
    (..., ([](InterruptPinConfig cfg) {
       Pin::InitializeInterrupt(cfg.pin, cfg.edge, cfg.pull);
       EnableExtiInterrupt(cfg.pin);
     })(Pins));
  }

  [[nodiscard]] static consteval bool
  PinInterruptActive(unsigned pin_num, hal::Edge edge) noexcept {
    return (... || ([pin_num, edge](InterruptPinConfig cfg) {
              return cfg.pin.num == pin_num
                     && (cfg.edge == edge || cfg.edge == hal::Edge::Both);
            })(Pins));
  }

  template <PinId Pin>
  constexpr void
  RegisterCallback(hstd::Callback<hal::Edge>& callback) noexcept {
    callbacks[GetIndex<Pin.num>()] = &callback;
  }

  template <unsigned Pin, hal::Edge Edge>
  constexpr void HandleInterrupt() noexcept {
    constexpr auto Idx = GetIndex<Pin>();
    if (callbacks[Idx] != nullptr) {
      (*callbacks[Idx])(Edge);
    }
  }

 private:
  template <unsigned Pin>
  consteval static std::size_t GetIndex() noexcept {
    using Ps = hstd::Values<InterruptPinConfig, Pins...>;
    return Ps::GetIndexBy([](auto cfg) { return cfg.pin.num == Pin; });
  }

  std::array<hstd::Callback<hal::Edge>*, sizeof...(Pins)> callbacks{};
};

export struct PinInterruptImplMarker {};

export template <typename M>
class PinInterrupt : public hal::UnusedPeripheral<PinInterrupt<M>> {
 public:
  [[nodiscard]] static consteval bool PinInterruptActive(unsigned,
                                                         hal::Edge) noexcept {
    std::unreachable();
    return false;
  }

  template <unsigned, hal::Edge>
  constexpr void HandleInterrupt() noexcept {
    std::unreachable();
  }
};

}   // namespace stm32g0