module;

#include <concepts>
#include <stm32u0xx_hal.h>

export module hal.stm32u0:nvic;

namespace stm32u0 {

namespace concepts {

export template <typename Impl>
concept CustomInterruptPriority = requires {
  { Impl::GetPriority(std::declval<IRQn_Type>()) } -> std::convertible_to<uint32_t>;
};

}   // namespace concepts

/**
 * Returns the interrupt priority defined by the custom interrupt priority type \c Impl. If \c Impl
 * does not provide an implementation of \c concepts::CustomInterruptPriority the function will
 * return 0.
 * @tparam Impl Implementation type.
 * @param irqn IRQ number.
 * @return Interrupt priority.
 */
template <typename Impl>
constexpr uint32_t GetIrqPrio(IRQn_Type irqn) noexcept {
  if constexpr (concepts::CustomInterruptPriority<Impl>) {
    return Impl::GetPriority(irqn);
  }

  return 0;
}

/**
 * @brief Enables the given interrupt.
 * @tparam Irqn Interrupt number to enable.
 * @tparam PrioImpl Interrupt priority implementation.
 */
export template <IRQn_Type Irqn, typename PrioImpl>
void EnableInterrupt() noexcept {
  NVIC_SetPriority(Irqn, GetIrqPrio<PrioImpl>(Irqn));
  NVIC_EnableIRQ(Irqn);
}

/**
 * @brief Enables the given interrupt.
 * @tparam PrioImpl Interrupt priority implementation.
 * @param irqn Interrupt to enable.
 */
export template <typename PrioImpl>
void EnableInterrupt(IRQn_Type irqn) {
  NVIC_SetPriority(irqn, GetIrqPrio<PrioImpl>(irqn));
  NVIC_EnableIRQ(irqn);
}

}   // namespace stm32u0
