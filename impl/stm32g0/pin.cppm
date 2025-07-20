module;

#include <utility>

#include <stm32g0xx_hal.h>

export module hal.stm32g0:pin;

import hal.abstract;

namespace stm32g0 {

/**
 * Enum containing the different pin ports on the MCU
 */
export enum class Port : uint8_t {
#ifdef GPIOA
  A,
#endif
#ifdef GPIOB
  B,
#endif
#ifdef GPIOC
  C,
#endif
#ifdef GPIOD
  D,
#endif
  E,
#ifdef GPIOF
  F
#endif
};

export using PinNum = uint8_t;

/**
 * Converts a port to a HAL port pointer
 * @param port Port to convert
 * @return Pointer to the GPIO port
 */
export constexpr auto* GetHalPort(Port port) noexcept {
  switch (port) {
#ifdef GPIOA
  case Port::A: return GPIOA;
#endif
#ifdef GPIOB
  case Port::B: return GPIOB;
#endif
#ifdef GPIOC
  case Port::C: return GPIOC;
#endif
#ifdef GPIOD
  case Port::D: return GPIOD;
#endif
#ifdef GPIOE
  case Port::E: return GPIOE;
#endif
#ifdef GPIOF
  case Port::F: return GPIOF;
#endif
  }

  std::unreachable();
}

/**
 * Converts a pin number to a HAL pin number
 * @param pin Pin number
 * @return HAL pin number
 */
export constexpr uint16_t GetHalPin(PinNum pin) noexcept {
  return 0b1U << pin;
}

void EnablePortClk(Port port) {
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

/**
 * ID of a pin
 */
export struct PinId {
  Port   port;
  PinNum num;

  [[nodiscard]] constexpr bool operator==(PinId rhs) const noexcept {
    return port == rhs.port && num == rhs.num;
  }

  [[nodiscard]] constexpr auto* hal_port() const noexcept {
    return GetHalPort(port);
  }

  [[nodiscard]] constexpr auto hal_pin() const noexcept {
    return GetHalPin(num);
  }
};

/**
 * Converts a HAL Edge to a STM32 HAL edge
 * @param edge Edge to convert
 * @return HAL edge value
 */
export constexpr uint32_t ToHalEdge(hal::Edge edge) noexcept {
  switch (edge) {
  case hal::Edge::Rising: return GPIO_MODE_IT_RISING;
  case hal::Edge::Falling: return GPIO_MODE_IT_FALLING;
  case hal::Edge::Both: return GPIO_MODE_IT_RISING_FALLING;
  default: std::unreachable();
  }
}

/**
 * Pin helper struct
 */
export struct Pin {
  /**
   * Initializes a pin
   * @param id Pin ID
   * @param dir Pin direction
   * @param pull Pin pull-up / pull-down
   * @param mode Pin mode
   */
  static void Initialize(PinId id, hal::PinDirection dir,
                         hal::PinPull pull = hal::PinPull::NoPull,
                         hal::PinMode mode = hal::PinMode::PushPull) noexcept {
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

  /**
   * Initializes a pin to an alternate function
   * @param id Pin ID
   * @param af Alternate function number
   * @param pull Pin pull-up / pull-down
   * @param mode Pin mode
   */
  static void
  InitializeAlternate(PinId id, unsigned af,
                      hal::PinPull pull = hal::PinPull::NoPull,
                      hal::PinMode mode = hal::PinMode::PushPull) noexcept {
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

  /**
   * Initializes a pin as an interrupt (EXTI) pin
   * @param id Pin ID
   * @param edge Edge to detect
   * @param pull Pin pull-up / pull-down
   */
  static void
  InitializeInterrupt(PinId id, hal::Edge edge,
                      hal::PinPull pull = hal::PinPull::NoPull) noexcept {
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
};

/**
 * General-purpose input
 */
export class Gpi {
 public:
  /**
   * Constructor
   * @param pin Pin ID
   * @param pull Pin pull
   * @param mode Pin mode
   */
  explicit Gpi(PinId pin, hal::PinPull pull = hal::PinPull::NoPull,
               hal::PinMode mode = hal::PinMode::PushPull) noexcept
      : pin{pin} {
    Pin::Initialize(pin, hal::PinDirection::Input, pull, mode);
  }

  /**
   * Reads the pin logic level
   * @return Pin logic level
   */
  [[nodiscard]] bool Read() const noexcept {
    return HAL_GPIO_ReadPin(pin.hal_port(), pin.hal_pin()) == GPIO_PIN_SET;
  }

 private:
  PinId pin;
};

/**
 * General-purpose output
 */
export class Gpo {
 public:
  /**
   * Constructor
   * @param pin Pin ID
   * @param pull Pin pull
   * @param mode Pin mode
   */
  explicit Gpo(PinId pin, hal::PinPull pull = hal::PinPull::NoPull,
               hal::PinMode mode = hal::PinMode::PushPull) noexcept
      : pin{pin} {
    Pin::Initialize(pin, hal::PinDirection::Output, pull, mode);
  }

  /**
   * Writes to the pin
   * @param value Pin value to write
   */
  void Write(bool value) const noexcept {
    HAL_GPIO_WritePin(pin.hal_port(), pin.hal_pin(),
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }

  /**
   * Toggles the pin
   */
  void Toggle() const noexcept {
    HAL_GPIO_TogglePin(pin.hal_port(), pin.hal_pin());
  }

 private:
  PinId pin;
};

// Validate concepts are implemented
static_assert(hal::Pin<Pin, PinId>);
static_assert(hal::Gpi<Gpi>);
static_assert(hal::ConstructibleGpo<Gpo, PinId>);

}   // namespace stm32g0