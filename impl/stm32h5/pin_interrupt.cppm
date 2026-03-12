module;

#include <string_view>
#include <utility>

#include <stm32h5xx_hal.h>

export module hal.stm32h5:pin_interrupt;

import hstd;

import hal.abstract;

import :pin;
import :nvic;

namespace stm32h5 {

export struct InterruptPinConfig {
  PinId        pin;
  hal::Edge    edge;
  hal::PinPull pull = hal::PinPull::NoPull;
};

[[nodiscard]] constexpr IRQn_Type GetExtiIrqNumber(PinNum pin_num) noexcept {
  switch (pin_num) {
  case 0: return EXTI0_IRQn;
  case 1: return EXTI1_IRQn;
  case 2: return EXTI2_IRQn;
  case 3: return EXTI3_IRQn;
  case 4: return EXTI4_IRQn;
  case 5: return EXTI5_IRQn;
  case 6: return EXTI6_IRQn;
  case 7: return EXTI7_IRQn;
  case 8: return EXTI8_IRQn;
  case 9: return EXTI9_IRQn;
  case 10: return EXTI10_IRQn;
  case 11: return EXTI11_IRQn;
  case 12: return EXTI12_IRQn;
  case 13: return EXTI13_IRQn;
  case 14: return EXTI14_IRQn;
  case 15: return EXTI15_IRQn;
  default: std::unreachable();
  }
}

template <typename PrioImpl>
void EnableExtiInterrupt(PinId pin) noexcept {
  const auto irqn = GetExtiIrqNumber(pin.num);
  EnableInterrupt<PrioImpl>(irqn);
}

template <typename Impl, unsigned Pin, hal::Edge Edge>
concept HasEdgeHandler =
    requires(Impl& impl) { impl.template HandleEdge<Pin, Edge>(); };

/**
 * @brief Pin Interrupts. Allows for defining interrupts on rising or falling
 * edges of pins.
 *
 * @tparam Impl Implementing type.
 * @tparam Pins Pins to configure the interrupt on.
 */
export template <typename Impl, InterruptPinConfig... Pins>
class PinInterruptImpl : public hal::UsedPeripheral {
 public:
  using PinType = PinId;

  static Impl& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  /**
   * @brief Constructor.
   */
  PinInterruptImpl() noexcept
      : hal::UsedPeripheral{} {
    (..., ([](InterruptPinConfig cfg) {
       Pin::InitializeInterrupt(cfg.pin, cfg.edge, cfg.pull);
       EnableExtiInterrupt<Impl>(cfg.pin);
     })(Pins));
  }

  /**
   * @brief Returns whether the given pin interrupt is active.
   *
   * @param pin_num Pin number to check.
   * @param edge Edge type to check.
   * @return Whether the pin interrupt is active.
   */
  [[nodiscard]] static consteval bool
  PinInterruptActive(unsigned pin_num, hal::Edge edge) noexcept {
    return (... || ([pin_num, edge](InterruptPinConfig cfg) {
              return cfg.pin.num == pin_num
                     && (cfg.edge == edge || cfg.edge == hal::Edge::Both);
            })(Pins));
  }

  template <unsigned Pin, hal::Edge Edge>
  constexpr void HandleInterrupt() noexcept {
    static_assert(HasEdgeHandler<Impl, Pin, Edge>);
    if constexpr (HasEdgeHandler<std::decay_t<Impl>, Pin, Edge>) {
      static_cast<Impl*>(this)->template HandleEdge<Pin, Edge>();
    }
  }
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

}   // namespace stm32h5