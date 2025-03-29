#include "pin.h"

namespace stm32g0 {

constexpr void EnablePortClk(Port port) {
  switch (port) {
#ifdef GPIOA
  case Port::A: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
#endif
#ifdef GPIOB
  case Port::B: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
#endif
#ifdef GPIOC
  case Port::C: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
#endif
#ifdef GPIOD
  case Port::D: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
#endif
#ifdef GPIOE
  case Port::E: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
#endif
#ifdef GPIOF
  case Port::F: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
#endif
  }
}

[[nodiscard]] constexpr uint32_t ToHalMode(hal::PinDirection dir,
                                           hal::PinMode      mode) {
  switch (dir) {
  case hal::PinDirection::Input: return GPIO_MODE_INPUT;
  case hal::PinDirection::Output:
    switch (mode) {
    case hal::PinMode::PushPull: return GPIO_MODE_OUTPUT_PP;
    case hal::PinMode::OpenDrain: return GPIO_MODE_OUTPUT_OD;
    }
    break;
  case hal::PinDirection::Analog: return GPIO_MODE_ANALOG;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t ToHalAlternateMode(hal::PinMode mode) {
  switch (mode) {
  case hal::PinMode::PushPull: return GPIO_MODE_AF_PP;
  case hal::PinMode::OpenDrain: return GPIO_MODE_AF_OD;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t ToHalPull(hal::PinPull pull) {
  switch (pull) {
  case hal::PinPull::NoPull: return GPIO_NOPULL;
  case hal::PinPull::PullUp: return GPIO_PULLUP;
  case hal::PinPull::PullDown: return GPIO_PULLDOWN;
  }

  std::unreachable();
}

void Pin::Initialize(stm32g0::PinId id, hal::PinDirection dir,
                     hal::PinPull pull, hal::PinMode mode) noexcept {
  EnablePortClk(id.port);

  GPIO_InitTypeDef init{
      .Pin       = id.hal_pin(),
      .Mode      = ToHalMode(dir, mode),
      .Pull      = ToHalPull(pull),
      .Speed     = GPIO_SPEED_FREQ_LOW,
      .Alternate = 0,
  };
  HAL_GPIO_Init(id.hal_port(), &init);
}

void Pin::InitializeAlternate(PinId id, unsigned int af, hal::PinPull pull,
                              hal::PinMode mode) noexcept {
  EnablePortClk(id.port);

  GPIO_InitTypeDef init{
      .Pin       = id.hal_pin(),
      .Mode      = ToHalAlternateMode(mode),
      .Pull      = ToHalPull(pull),
      .Speed     = GPIO_SPEED_FREQ_VERY_HIGH,
      .Alternate = af,
  };
  HAL_GPIO_Init(id.hal_port(), &init);
}

void Pin::InitializeInterrupt(PinId id, hal::Edge edge,
                              hal::PinPull pull) noexcept {
  EnablePortClk(id.port);

  GPIO_InitTypeDef init{
      .Pin       = id.hal_pin(),
      .Mode      = ToHalEdge(edge),
      .Pull      = ToHalPull(pull),
      .Speed     = GPIO_SPEED_FREQ_VERY_HIGH,
      .Alternate = 0,
  };
  HAL_GPIO_Init(id.hal_port(), &init);
}

Gpo::Gpo(stm32g0::PinId pin, hal::PinPull pull, hal::PinMode mode) noexcept
    : pin{pin} {
  Pin::Initialize(pin, hal::PinDirection::Output, pull, mode);
}

void Gpo::Write(bool value) const noexcept {
  HAL_GPIO_WritePin(pin.hal_port(), pin.hal_pin(),
                    value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Gpo::Toggle() const noexcept {
  HAL_GPIO_TogglePin(pin.hal_port(), pin.hal_pin());
}

Gpi::Gpi(stm32g0::PinId pin, hal::PinPull pull, hal::PinMode mode) noexcept
    : pin{pin} {
  Pin::Initialize(pin, hal::PinDirection::Input, pull, mode);
}

bool Gpi::Read() const noexcept {
  return HAL_GPIO_ReadPin(pin.hal_port(), pin.hal_pin()) == GPIO_PIN_SET;
}

}   // namespace stm32g0