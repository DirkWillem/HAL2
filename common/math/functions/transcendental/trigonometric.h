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
/**
 * Shifts x to the range [-pi, +pi]
 * @tparam F Floating point type of x
 * @param x Value to shift
 * @return Shifted value
 */
constexpr F ShiftToPmPi(F x) {
  const F pi = static_cast<F>(M_PI);
  while (x > pi) {
    x -= 2 * pi;
  }

  while (x < -pi) {
    x += 2 * pi;
  }

  return x;
}

template <std::floating_point F>
/**
 * Approximates sin(x) using a taylor expansion around x=0
 * @tparam F Floating point type
 * @param x Value to calculate sine approximation of
 * @param order Taylor series order
 * @return Approximation of sin(x)
 */
constexpr F SinTaylorApprox(F x, unsigned order) noexcept {
  unsigned int factorial = 1;
  const F      x2        = x * x;

  // Start at order 1 term
  F xn     = x;
  F result = x;

  // Compute higher-order terms
  bool positive = false;
  for (int i = 3; i <= order; i += 2) {
    factorial *= i * (i - 1);
    xn *= x2;

    if (positive) {
      result += xn / static_cast<F>(factorial);
    } else {
      result -= xn / static_cast<F>(factorial);
    }

    positive = !positive;
  }

  return result;
}

template <std::floating_point F = float>
/**
 * Approximates cos(x) using a taylor expansion around x=0
 * @tparam F Floating point type
 * @param x Value to calculate cosine approximation of
 * @param order Taylor series order
 * @return Approximation of cos(x)
 */
constexpr F CosTaylorApprox(F x, unsigned order) noexcept {
  unsigned int factorial = 1;
  const F      x2        = x * x;

  // Start at order 0 term
  F xn     = static_cast<F>(1.0);
  F result = static_cast<F>(1.0);

  // Compute higher-order terms
  bool positive = false;
  for (int i = 2; i <= order; i += 2) {
    factorial *= i * (i - 1);
    xn *= x2;

    if (positive) {
      result += xn / static_cast<F>(factorial);
    } else {
      result -= xn / static_cast<F>(factorial);
    }

    positive = !positive;
  }

  return result;
}

template <bool CE, std::floating_point F = float,
          FuncSettings S = FuncSettings{}>
constexpr F SinImpl(F x) noexcept {
  constexpr auto implementation =
      ChooseImplementation(S.implementation, CE, true);

  if constexpr (implementation == ChosenImpl::StdLib) {
    return std::sin(x);
  } else if constexpr (implementation == ChosenImpl::Taylor) {
    constexpr F Pi0_25 = static_cast<F>(0.25 * M_PI);
    constexpr F Pi0_5  = static_cast<F>(0.5 * M_PI);
    constexpr F Pi0_75 = static_cast<F>(0.75 * M_PI);
    constexpr F Pi     = static_cast<F>(M_PI);

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
    static_assert(std::is_same_v<F, float>,
                  "CMSIS-DSP sin is only supported for f32");

    if constexpr (std::is_same_v<F, float>) {
      return arm_sin_f32(x);
    }
  } else {
    std::unreachable();
  }
}

template <bool CE, std::floating_point F = float,
          FuncSettings S = FuncSettings{}>
constexpr F CosImpl(F x) noexcept {
  constexpr auto implementation =
      ChooseImplementation(S.implementation, CE, true);

  if constexpr (implementation == ChosenImpl::StdLib) {
    return std::cos(x);
  } else if constexpr (implementation == ChosenImpl::Taylor) {
    constexpr F Pi0_25 = static_cast<F>(0.25 * M_PI);
    constexpr F Pi0_5  = static_cast<F>(0.5 * M_PI);
    constexpr F Pi0_75 = static_cast<F>(0.75 * M_PI);
    constexpr F Pi     = static_cast<F>(M_PI);

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
    static_assert(std::is_same_v<F, float>,
                  "CMSIS-DSP cos is only supported for f32");

    if constexpr (std::is_same_v<F, float>) {
      return arm_cos_f32(x);
    }
  } else {
    std::unreachable();
  }
}

template <bool CE, std::floating_point F = float,
          FuncSettings S = FuncSettings{}>
constexpr std::pair<F, F> SinCosImpl(F x) noexcept {
  constexpr auto implementation =
      ChooseImplementation(S.implementation, CE, true);

  if constexpr (implementation == ChosenImpl::StdLib) {
    return {std::sin(x), std::cos(x)};
  } else if constexpr (implementation == ChosenImpl::Taylor) {
    return {SinImpl<CE, F, S>(x), CosImpl<CE, F, S>(x)};
  } else if constexpr (implementation == ChosenImpl::CmsisDsp) {
    static_assert(std::is_same_v<F, float>,
                  "CMSIS-DSP sin/cos is only supported for f32");

    if constexpr (std::is_same_v<F, float>) {
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

template <std::floating_point F = float, FuncSettings S = FuncSettings{}>
/**
 * Computes the sine using a taylor expansion
 * @param x Value to compute the sine of
 * @return Approximation of the sine of x
 */
constexpr F Sin(F x) noexcept {
  if (std::is_constant_evaluated()) {
    return detail::SinImpl<true, F, S>(x);
  } else {
    return detail::SinImpl<false, F, S>(x);
  }
}

template <std::floating_point F = float, FuncSettings S = FuncSettings{}>
/**
 * Computes the cosine of x
 * @param x Value to compute the cosine of
 * @return Approximation of the cosine of x
 */
constexpr F Cos(F x) noexcept {
  if (std::is_constant_evaluated()) {
    return detail::CosImpl<true, F, S>(x);
  } else {
    return detail::CosImpl<false, F, S>(x);
  }
}

template <std::floating_point F = float, FuncSettings S = FuncSettings{}>
/**
 * Computes the sine and cosine of x
 * @param x Value to compute the cosine of
 * @return Approximation of the cosine of x
 */
constexpr std::pair<F, F> SinCos(F x) noexcept {
  if (std::is_constant_evaluated()) {
    return detail::SinCosImpl<true, F, S>(x);
  } else {
    return detail::SinCosImpl<false, F, S>(x);
  }
}

}   // namespace math
