module;

#include <concepts>

#include <stm32h5xx_hal.h>

export module hal.stm32h5:nvic;

namespace stm32h5 {

namespace concepts {

export template <typename Impl>
concept CustomInterruptPriority = requires {
  {
    Impl::GetPriority(std::declval<IRQn_Type>())
  } -> std::convertible_to<uint32_t>;
};

}   // namespace concepts

template <typename Impl>
consteval uint32_t GetIrqPrio(IRQn_Type irqn) noexcept {
  if constexpr (concepts::CustomInterruptPriority<Impl>) {
    return static_cast<uint32_t>(irqn);
  }

  return 0;
}

export template<IRQn_Type Irqn, typename PrioImpl>
void EnableInterrupt() noexcept {
  NVIC_SetPriority(Irqn, GetIrqPrio<PrioImpl>(Irqn));
  NVIC_EnableIRQ(Irqn);
}

}   // namespace stm32h5
