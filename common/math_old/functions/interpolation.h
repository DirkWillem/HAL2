#pragma once

#include <math/concepts.h>

namespace math {

template <Real R>
constexpr R LinearInterpolate(R x1, R x2, R y1, R y2,
                              R xq) {
  const auto dx = x2 - x1;
  const auto dy = y2 - y1;

  return y1 + dy / dx * (xq - x1);
}

template <Real R>
constexpr R BilinearInterpolateUnitSquare(R y00, R y10, R y01, R y11, R x,
                                          R y) {
  const R a00 = y00;
  const R a10 = y10 - y00;
  const R a01 = y01 - y00;
  const R a11 = y11 - y01 - y10 + y00;

  return a00 + (a10 * x) + (a01 * y) + (a11 * x * y);
}

}   // namespace math