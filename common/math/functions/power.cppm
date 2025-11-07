module;

#include <cmath>
#include <utility>

#include <arm_math.h>

export module math:functions.power;

import :concepts;
import :settings;

namespace math {

/**
 * @brief Computes the square root of \c x.
 * @tparam Num Numeric type.
 * @tparam S Function settings.
 * @param x Operand.
 * @return Square root of \c x.
 */
export template <concepts::Number Num, Settings S = {}>
Num Sqrt(Num x) noexcept {
  if constexpr (S.implementation == Implementation::Default) {
    return std::sqrt(x);
  } else if constexpr (S.implementation == Implementation::CmsisDsp) {
    static_assert(std::is_same_v<Num, float>,
                  "Sqrt is only supported for float when using CMSIS-DSP");

    if constexpr (std::is_same_v<Num, float>) {
      float result{};
      arm_sqrt_f32(x, &result);
      return result;
    } else {
      std::unreachable();
    }
  }
}

}   // namespace math