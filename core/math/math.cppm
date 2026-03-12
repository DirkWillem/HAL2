module;

#include <concepts>
#include <cstdint>
#include <numbers>

export module math;

export import :concepts;
export import :settings;
export import :linalg.vector;
export import :functions.interpolation;
export import :functions.trigonometric;
export import :functions.power;
export import :geometry.coordinate;
export import :lut;

namespace math {

export inline constexpr auto Pi    = std::numbers::pi_v<float>;
export inline constexpr auto TwoPi = 2.F * Pi;

export template <std::unsigned_integral T>
constexpr T FastFloor(float f) {
  return static_cast<T>(f);
}

export template <std::unsigned_integral T>
constexpr T FastCeil(float f) {
  const auto i = static_cast<T>(f);
  return f > static_cast<float>(i) ? i + 1 : i;
}

}   // namespace math