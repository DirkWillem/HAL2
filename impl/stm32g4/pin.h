#pragma once

#include <utility>

#include <stm32g4xx_hal.h>

#include <hal/pin.h>

namespace stm32g4 {

/**
 * Enum containing the different pin ports on the MCU
 */
enum class Port : uint8_t {
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
#ifdef GPIOE
  E,
#endif
#ifdef GPIOF
  F,
#endif
#ifdef GPIOG
  G,
#endif
};

using PinNum = uint8_t;

/**
 * Converts a port to a HAL port pointer
 * @param port Port to convert
 * @return Pointer to the GPIO port
 */
static constexpr auto* GetHalPort(Port port) noexcept {
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
#ifdef GPIOG
  case Port::G: return GPIOG;
#endif
  }

  std::unreachable();
}

/**
 * Converts a pin number to a HAL pin number
 * @param pin Pin number
 * @return HAL pin number
 */
static constexpr uint16_t GetHalPin(PinNum pin) noexcept {
  return 0b1U << pin;
}

/**
 * ID of a pin
 */
struct PinId {
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
 * Pin ID helper macro
 */
#define PIN(PORT, NUM)         \
  ::stm32g4::PinId {           \
    ::stm32g4::Port::PORT, NUM \
  }

/**
 * Pin helper struct
 */
struct Pin {
  /**
   * Initializes a pin
   * @param id Pin ID
   * @param dir Pin direction
   * @param pull Pin pull-up / pull-down
   * @param mode Pin mode
   */
  static void Initialize(PinId id, hal::PinDirection dir,
                         hal::PinPull pull = hal::PinPull::NoPull,
                         hal::PinMode mode = hal::PinMode::PushPull) noexcept;

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
                      hal::PinMode mode = hal::PinMode::PushPull) noexcept;
};

/**
 * General-purpose input
 */
class Gpi {
 public:
  /**
   * Constructor
   * @param pin Pin ID
   * @param pull Pin pull
   * @param mode Pin mode
   */
  explicit Gpi(PinId pin, hal::PinPull pull = hal::PinPull::NoPull,
               hal::PinMode mode = hal::PinMode::PushPull) noexcept;

  /**
   * Reads the pin logic level
   * @return Pin logic level
   */
  [[nodiscard]] bool Read() const noexcept;

 private:
  PinId pin;
};

/**
 * General-purpose output
 */
class Gpo {
 public:
  /**
   * Constructor
   * @param pin Pin ID
   * @param pull Pin pull
   * @param mode Pin mode
   */
  explicit Gpo(PinId pin, hal::PinPull pull = hal::PinPull::NoPull,
               hal::PinMode mode = hal::PinMode::PushPull) noexcept;

  /**
   * Writes to the pin
   * @param value Pin value to write
   */
  void Write(bool value) const noexcept;

  /**
   * Toggles the pin
   */
  void Toggle() const noexcept;

 private:
  PinId pin;
};

// Validate concepts are implemented
static_assert(hal::Pin<Pin, PinId>);
static_assert(hal::Gpi<Gpi, PinId>);
static_assert(hal::Gpo<Gpo>);
static_assert(hal::ConstructibleGpo<Gpo, PinId>);

}   // namespace stm32g4