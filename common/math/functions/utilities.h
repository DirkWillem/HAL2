#include <cmath>

#include <math/concepts.h>

namespace math {

template <Real R>
constexpr R RadToDeg(R rad) noexcept {
  return rad * static_cast<R>(180.0 / M_PI);
}

template <Real R>
constexpr R DegToRad(R rad) noexcept {
  constexpr R Scale{M_PI / 180.0};
  return rad * Scale;
}

}   // namespace math