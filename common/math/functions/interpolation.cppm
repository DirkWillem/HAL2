module;

#include <cmath>
#include <utility>

#include <arm_math.h>

export module math:functions.interpolation;

import :concepts;
import :settings;

namespace math {

/**
 * @brief Performs linear interpolation given 2 points and a desired X
 * coordinate to interpolate at.
 * @tparam R Real numeric type.
 * @param x1 X coordinate of first interpolation point.
 * @param x2 X coordinate of second interpolation point.
 * @param y1 Y coordinate of first interpolation point.
 * @param y2 Y coordinate of second interpolation point.
 * @param xq X coordinate to interpolate at
 * @return Interpolated y coordinate at \c xq.
 */
template <concepts::Real R>
constexpr R LinearInterpolate(R x1, R x2, R y1, R y2, R xq) {
  const auto dx = x2 - x1;
  const auto dy = y2 - y1;

  return y1 + dy / dx * (xq - x1);
}

/**
 * @brief Interpolates a value given a coordinate within the unit square and the
 * values at the corner points of the unit square.
 * @tparam R Real numeric type.
 * @param y00 Value at coordinate <c>(0, 0)</c>.
 * @param y10 Value at coordinate <c>(1, 0)</c>.
 * @param y01 Value at coordinate <c>(0, 1)</c>.
 * @param y11 Value at coordinate <c>(1 ,1)</c>.
 * @param x X coordinate to interpolate at.
 * @param y Y coordinate to interpolate at.
 * @return Interpolated value.
 */
export template <concepts::Real R>
constexpr R BilinearInterpolateUnitSquare(R y00, R y10, R y01, R y11, R x,
                                          R y) {
  const R a00 = y00;
  const R a10 = y10 - y00;
  const R a01 = y01 - y00;
  const R a11 = y11 - y01 - y10 + y00;

  return a00 + (a10 * x) + (a01 * y) + (a11 * x * y);
}

}   // namespace math