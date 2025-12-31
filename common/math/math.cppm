module;

#include <concepts>
#include <cstdint>

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

export template <std::unsigned_integral T>
constexpr T FastCeil(float f) {
  const auto i = static_cast<T>(f);
  return f > static_cast<float>(i) ? i + 1 : i;
}

}   // namespace math