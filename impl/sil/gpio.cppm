module;

#include <cstdint>
#include <string>

export module hal.sil:gpio;

import hal.abstract;

namespace sil {

/**
 * Direction of a GPIO
 */
export enum class GpioDirection {
  Input,    //!< GPI
  Output,   //!< GPO
};

/**
 * General purpose input-output, can serve as a GPI, GPO or GPIO
 */
export class Gpio {
 public:
  /**
   * Constructor
   * @param name GPIO name
   * @param direction Direction of the GPIO
   * @param initial_state Initial state of the GPIO
   */
  Gpio(std::string name, GpioDirection direction,
       bool initial_state = false) noexcept
      : name{name}
      , direction{direction}
      , state{initial_state} {}

  /**
   * Reads the pin logic level
   * @return Pin logic level
   */
  [[nodiscard]] bool Read() const noexcept { return state; }

  /**
   * Writes to the pin
   * @param value Pin value to write
   */
  void Write(bool value) noexcept { state = value; }

  /**
   * Toggles the pin
   */
  void Toggle() noexcept { state = !state; }

  /**
   * Returns the GPIO name
   * @return GPIO name
   */
  [[nodiscard]] const std::string& GetName() const& noexcept { return name; }

 private:
  GpioDirection direction;
  bool          state;
  std::string   name;
};

static_assert(hal::Gpi<Gpio>);
static_assert(hal::Gpo<Gpio>);

}   // namespace sil
