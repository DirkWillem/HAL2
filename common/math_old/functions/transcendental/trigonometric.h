#pragma once

#include <cmath>
#include <utility>

#include <math/concepts.h>
#include <math/functions/common.h>

#ifdef HAS_CMSIS_DSP
#include <arm_math.h>
#endif

namespace math {

namespace detail {

template <Real R>
/**
 * Shifts x to the range [-pi, +pi]
 * @tparam R Real type of x
 * @param x Value to shift
 * @return Shifted value
 */
constexpr R ShiftToPmPi(R x) {
  const R pi     = R{M_PI};
  const R two_pi = R{M_PI * 2};
  while (x > pi) {
    x -= two_pi;
  }

  if constexpr (!fp::is_unsigned_fixed_point_v<R>) {
    while (x < -pi) {
      x += two_pi;
    }
  }

  return x;
}

template <Real R>
/**
 * Approximates sin(x) using a taylor expansion around x=0
 * @tparam F Floating point type
 * @param x Value to calculate sine approximation of
 * @param order Taylor series order
 * @return Approximation of sin(x)
 */
constexpr R SinTaylorApprox(R x, unsigned order) noexcept {
  unsigned int factorial = 1;
  const R      x2        = x * x;

  // Start at order 1 term
  R xn     = x;
  R result = x;

  // Compute higher-order terms
  bool positive = false;
  for (int i = 3; i <= order; i += 2) {
    factorial *= i * (i - 1);
    xn *= x2;

    if (positive) {
      result += xn / static_cast<R>(factorial);
    } else {
      result -= xn / static_cast<R>(factorial);
    }

    positive = !positive;
  }

  return result;
}

template <Real R = float>
/**
 * Approximates cos(x) using a taylor expansion around x=0
 * @tparam R Real type
 * @param x Value to calculate cosine approximation of
 * @param order Taylor series order
 * @return Approximation of cos(x)
 */
constexpr R CosTaylorApprox(R x, unsigned order) noexcept {
  unsigned int factorial = 1;
  const R      x2        = x * x;

  // Start at order 0 term
  R xn{1};
  R result{1};

  // Compute higher-order terms
  bool positive = false;
  for (int i = 2; i <= order; i += 2) {
    factorial *= i * (i - 1);
    xn *= x2;

    if (positive) {
      result += xn / static_cast<R>(factorial);
    } else {
      result -= xn / static_cast<R>(factorial);
    }

    positive = !positive;
  }

  return result;
}

template <bool CE, Real R = float, FuncSettings S = FuncSettings{}>
constexpr R SinImpl(R x) noexcept {
  constexpr auto implementation =
      ChooseImplementation(S.implementation, CE, true, ConstEvalImpl::Taylor);

  if constexpr (implementation == ChosenImpl::StdLib) {
    return std::sin(x);
  } else if constexpr (implementation == ChosenImpl::Taylor) {
    constexpr R Pi0_25{0.25 * M_PI};
    constexpr R Pi0_5{0.5 * M_PI};
    constexpr R Pi0_75{0.75 * M_PI};
    constexpr R Pi{M_PI};

    // Shift x to the range +/- pi
    x = ShiftToPmPi(x);

    // Check if there is a closer approximation than regular taylor expansion of
    // sin(x)
    if (x > Pi0_25 && x <= Pi0_75) {
      return CosTaylorApprox(x - Pi0_5, S.taylor_series_order);
    } else if (x >= -Pi0_75 && x < -Pi0_25) {
      return -CosTaylorApprox(x + Pi0_5, S.taylor_series_order);
    } else if (x > Pi0_75) {
      return -SinTaylorApprox(x - Pi, S.taylor_series_order);
    } else if (x < -Pi0_75) {
      return -SinTaylorApprox(x + Pi, S.taylor_series_order);
    }

    // No closer approximation, return taylor approximation around 0
    return SinTaylorApprox(x, S.taylor_series_order);
  } else if constexpr (implementation == ChosenImpl::CmsisDsp) {
    if constexpr (std::is_same_v<R, float>) {
      return arm_sin_f32(x);
    } else {
      std::unreachable();
    }
  } else {
    std::unreachable();
  }
}

template <bool CE, Real R = float, FuncSettings S = FuncSettings{}>
constexpr R CosImpl(R x) noexcept {
  constexpr auto implementation =
      ChooseImplementation(S.implementation, CE, true, ConstEvalImpl::Taylor);

  if constexpr (implementation == ChosenImpl::StdLib) {
    return std::cos(x);
  } else if constexpr (implementation == ChosenImpl::Taylor) {
    constexpr R Pi0_25{0.25 * M_PI};
    constexpr R Pi0_5{0.5 * M_PI};
    constexpr R Pi0_75{0.75 * M_PI};
    constexpr R Pi{M_PI};

    // Shift x to the range +/- pi
    x = ShiftToPmPi(x);

    // Check if there is a closer approximation than regular taylor expansion of
    // sin(x)
    if (x > Pi0_25 && x <= Pi0_75) {
      return -SinTaylorApprox(x - Pi0_5, S.taylor_series_order);
    } else if (x >= -Pi0_75 && x < -Pi0_25) {
      return SinTaylorApprox(x + Pi0_5, S.taylor_series_order);
    } else if (x > Pi0_75) {
      return -CosTaylorApprox(x - Pi, S.taylor_series_order);
    } else if (x < -Pi0_75) {
      return -CosTaylorApprox(x + Pi, S.taylor_series_order);
    }

    // No closer approximation, return taylor approximation around 0
    return CosTaylorApprox(x, S.taylor_series_order);
  } else if constexpr (implementation == ChosenImpl::CmsisDsp) {
    static_assert(std::is_same_v<R, float>,
                  "CMSIS-DSP cos is only supported for f32");

    if constexpr (std::is_same_v<R, float>) {
      return arm_cos_f32(x);
    }
  } else {
    std::unreachable();
  }
}

template <bool CE, Real R = float, FuncSettings S = FuncSettings{}>
constexpr std::pair<R, R> SinCosImpl(R x) noexcept {
  constexpr auto implementation =
      ChooseImplementation(S.implementation, CE, true, ConstEvalImpl::Taylor);

  if constexpr (implementation == ChosenImpl::StdLib) {
    return {std::sin(x), std::cos(x)};
  } else if constexpr (implementation == ChosenImpl::Taylor) {
    return {SinImpl<CE, R, S>(x), CosImpl<CE, R, S>(x)};
  } else if constexpr (implementation == ChosenImpl::CmsisDsp) {
    static_assert(std::is_same_v<R, float>,
                  "CMSIS-DSP sin/cos is only supported for f32");

    if constexpr (std::is_same_v<R, float>) {
      float s;
      float c;
      arm_sin_cos_f32(x, &s, &c);
      return {s, c};
    }
  } else {
    std::unreachable();
  }
}

}   // namespace detail

template <Real R = float, FuncSettings S = FuncSettings{}>
/**
 * Computes the sine using a taylor expansion
 * @param x Value to compute the sine of
 * @return Approximation of the sine of x
 */
constexpr R Sin(R x) noexcept {
  if (std::is_constant_evaluated()) {
    return detail::SinImpl<true, R, S>(x);
  } else {
    return detail::SinImpl<false, R, S>(x);
  }
}

template <Real R = float, FuncSettings S = FuncSettings{}>
/**
 * Computes the cosine of x
 * @param x Value to compute the cosine of
 * @return Approximation of the cosine of x
 */
constexpr R Cos(R x) noexcept {
  if (std::is_constant_evaluated()) {
    return detail::CosImpl<true, R, S>(x);
  } else {
    return detail::CosImpl<false, R, S>(x);
  }
}

template <Real R = float, FuncSettings S = FuncSettings{}>
/**
 * Computes the sine and cosine of x
 * @param x Value to compute the cosine of
 * @return Approximation of the cosine of x
 */
constexpr std::pair<R, R> SinCos(R x) noexcept {
  if (std::is_constant_evaluated()) {
    return detail::SinCosImpl<true, R, S>(x);
  } else {
    return detail::SinCosImpl<false, R, S>(x);
  }
}

}   // namespace math
