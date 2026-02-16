module;

#include <cmath>
#include <utility>

#include <arm_math.h>

export module math:functions.trigonometric;

import :concepts;
import :settings;

namespace math {

/**
 * @brief Computes the arc tangent of y / x using the signs of arguments to
 * determine the correct quadrant.
 * @tparam Num Numeric type.
 * @tparam S Function settings.
 * @param y Y coordinate.
 * @param x X coordinate.
 * @return atan2(y, x).
 */
export template <concepts::Number Num, Settings S = {}>
Num Atan2(Num y, Num x) noexcept {
  if constexpr (S.implementation == Implementation::Default) {
    return std::atan2(y, x);
  } else if constexpr (S.implementation == Implementation::CmsisDsp) {
    static_assert(std::is_same_v<Num, float>,
                  "Atan2 is only supported for float when using CMSIS-DSP");

    if constexpr (std::is_same_v<Num, float>) {
      float result{};
      arm_atan2_f32(y, x, &result);
      return result;
    } else {
      std::unreachable();
    }
  }
}

}   // namespace math