#pragma once

#include <math/math_config.h>

namespace math {

/**
 * Implementation preference for evaluation of a mathematical function
 */
enum class Implementation {
  /** Use the default implementation for the current context */
  Default,
  /** Lways use the standard library implementation from <cmath> */
  ForceStandardLibrary,
  /** Always use a taylor series approximation */
  ForceTaylorSeriesApproximation,
  /** Always use a Newton-Raphson approximation */
  ForceNewtonRaphsonApproximation,
  /** Always use the CMSIS-DSP implementation */
  ForceCmsisDsp,
};

/**
 * Settings for how a mathematical function should be evaluated
 */
struct FuncSettings {
  /** Preferred implementation of the function */
  Implementation implementation = Implementation::Default;
  /**
   * In the case where a taylor series approximation is used,
   * the order of the approximation
   */
  unsigned taylor_series_order = 9;
  /** Number of iteration sto use in case of using the Newton-Raphson method */
  unsigned newton_raphson_iterations = 20;
};

namespace detail {

enum class ConstEvalImpl { Taylor, NewtonRaphson };

enum class ChosenImpl { StdLib, Taylor, NewtonRaphson, CmsisDsp };

constexpr ChosenImpl
ChooseImplementation(Implementation preference, bool is_constant_evaluated,
                     bool supported_by_cmsis_dsp, ConstEvalImpl const_eval_impl) noexcept {
  switch (preference) {
  case Implementation::Default:
    if (is_constant_evaluated) {
      switch (const_eval_impl) {
      case ConstEvalImpl::Taylor:
        return ChosenImpl::Taylor;
      case ConstEvalImpl::NewtonRaphson:
        return ChosenImpl::NewtonRaphson;
      default:
        std::unreachable();
      }
    } else if (CmsisDspAvailable && supported_by_cmsis_dsp) {
      return ChosenImpl::CmsisDsp;
    }
    return ChosenImpl::StdLib;

  case Implementation::ForceStandardLibrary: return ChosenImpl::StdLib;
  case Implementation::ForceTaylorSeriesApproximation:
    return ChosenImpl::Taylor;
  case Implementation::ForceNewtonRaphsonApproximation:
    return ChosenImpl::NewtonRaphson;
  case Implementation::ForceCmsisDsp:
    if (!CmsisDspAvailable || !supported_by_cmsis_dsp) {
      std::unreachable();
    }

    return ChosenImpl::CmsisDsp;
  }
}

}   // namespace detail

}   // namespace math