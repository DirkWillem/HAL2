#pragma once

#include <cmath>
#include <utility>

#include <math/functions/common.h>

#ifdef HAS_CMSIS_DSP
#include <arm_math.h>
#endif

namespace math {

namespace detail {

template <std::floating_point F>
constexpr F SqrtNewtonRaphsonApprox(F x, unsigned iterations) noexcept {
  if (x < 0) {
    return NAN;
  } else if (x == 0) {
    return 0.F;
  }

  F xn = 0.5F*(x+1);

  for (auto i = 0; i < iterations; i++) {
    xn = 0.5F * (xn + x / xn);
  }

  return xn;
}

template <bool CE, std::floating_point F, FuncSettings S>
constexpr F SqrtImpl(F x) noexcept {
  constexpr auto implementation = ChooseImplementation(
      S.implementation, CE, true, ConstEvalImpl::NewtonRaphson);

  if constexpr (implementation == ChosenImpl::NewtonRaphson) {
    return SqrtNewtonRaphsonApprox(x, S.newton_raphson_iterations);
  } else if constexpr (implementation == ChosenImpl::StdLib) {
    return std::sqrt(x);
  } else if constexpr (implementation == ChosenImpl::CmsisDsp) {
    static_assert(
        std::is_same_v<F, float>,
        "CMSIS-DSP implementation of Sqrt is only available for float");

    if constexpr (std::is_same_v<F, float>) {
      float y;
      if (arm_sqrt_f32(x, &y) != ARM_MATH_SUCCESS) {
        return NAN;
      }
      return y;
    } else {
      std::unreachable();
    }
  } else {
    std::unreachable();
  }
}

}   // namespace detail

template <std::floating_point F, FuncSettings S = FuncSettings{}>
constexpr F Sqrt(F x) noexcept {
  if consteval {
    return detail::SqrtImpl<true, F, S>(x);
  } else {
    return detail::SqrtImpl<false, F, S>(x);
  }
}

}   // namespace math
