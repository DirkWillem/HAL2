module;

#include <functional>
#include <optional>
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
 * Possible edges of a GPIO pin
 */
export enum class Edge {
  Rising  = 0,   //!< Rising edge
  Falling = 1,   //!< Falling edge
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
      , dir{direction}
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
  void Write(bool value) noexcept {
    if (value != state && edge_callback.has_value()) {
      (*edge_callback)(value ? Edge::Rising : Edge::Falling);
    }
    state = value;
  }

  /**
   * Toggles the pin
   */
  void Toggle() noexcept {
    const auto new_state = !state;
    if (edge_callback.has_value()) {
      (*edge_callback)(new_state ? Edge::Rising : Edge::Falling);
    }
    state = new_state;
  }

  /**
   * Sets the edge callback for the GPIO
   * @param cb Callback to set
   */
  void SetEdgeCallback(std::function<void(Edge)> cb) { edge_callback = cb; }

  /**
   * Clears the edge callback of the GPIO
   */
  void ClearEdgeCallback() { edge_callback = std::nullopt; }

  /**
   * Returns the GPIO direction
   * @return GPIO direction
   */
  [[nodiscard]] constexpr GpioDirection direction() const noexcept {
    return dir;
  }

  /**
   * Returns the GPIO name
   * @return GPIO name
   */
  [[nodiscard]] const std::string& GetName() const& noexcept { return name; }

 private:
  GpioDirection dir;
  bool          state;
  std::string   name;

  std::optional<std::function<void(Edge)>> edge_callback{std::nullopt};
};

static_assert(hal::Gpi<Gpio>);
static_assert(hal::Gpo<Gpio>);

}   // namespace sil
