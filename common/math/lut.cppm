module;

#include <array>
#include <span>
#include <tuple>
#include <utility>

#include <dsp/fast_math_functions.h>
#include <dsp/interpolation_functions.h>

export module math:lut;

import hstd;

import :concepts;

namespace math {

template <concepts::Number R>
struct LutType;

template <>
struct LutType<float> {
  using Type = arm_linear_interp_instance_f32;
};

/**
 * @brief Linear interpolation lookup table with uniformly spaced samples.
 * @tparam R Numeric type.
 * @tparam N Number of entries in the LUT.
 */
export template <concepts::Number R, std::size_t N>
class Lut {
 public:
  /**
   * @brief Constructor.
   * @param x_min Lowest X coordinate.
   * @param x_max Highest X coordinate.
   */
  explicit Lut(float x_min = 0.F, float x_max = 1.F) noexcept
      : xr{x_min, x_max}
      , interp{
            .nValues  = N,
            .x1       = x_min,
            .xSpacing = (x_max - x_min) / static_cast<float>(N - 1),
            .pYData   = d.data(),
        } {}

  /**
   * @brief Returns a view over the Y-axis data.
   * @return View over Y data.
   */
  [[nodiscard]] std::span<R> data() & noexcept { return std::span{d}; }

  /**
   * @brief Returns the X lower bound.
   * @return X lower bound.
   */
  [[nodiscard]] R x_min() const noexcept { return xr.first; }

  /**
   * @brief Returns the X higher bound.
   * @return X higher bound.
   */
  [[nodiscard]] R x_max() const noexcept { return xr.second; }

  /**
   * @brief Returns the X spacing.
   * @return X spacing.
   */
  [[nodiscard]] R dx() const noexcept { return interp.xSpacing; }

  /**
   * @brief Samples the LUT.
   * @param xq X coordinate to sample at.
   * @return Sampled Y value.
   */
  [[nodiscard]] float Sample(const float xq) const noexcept {
    return arm_linear_interp_f32(&interp, xq);
  }

 private:
  std::array<R, N> d{};
  std::pair<R, R>  xr;
  LutType<R>::Type interp;
};

/**
 * @brief Creates a sine LUT.
 * @tparam R Numeric type.
 * @tparam N Number of LUT entries.
 * @param lut LUT to fill.
 */
export template <concepts::Number R, std::size_t N>
void CreateSinLut(Lut<R, N>& lut) noexcept {
  const auto dx = lut.dx();
  auto       y  = lut.data();

  for (std::size_t i = 0; i < N; ++i) {
    if constexpr (std::is_same_v<R, float>) {
      y[i] = arm_sin_f32(static_cast<float>(i) * dx);
    } else {
      std::unreachable();
    }
  }
}

/**
 * @brief Creates a sine LUT with explicit x and y bounds, possibly different
 * from those in the LUT.
 * @tparam R Numeric type.
 * @tparam N Number of LUT entries.
 * @param lut LUT to fill.
 * @param x_min Minimum X coordinate.
 * @param x_max Maximum X coordinate.
 */
export template <concepts::Number R, std::size_t N>
void CreateSinLut(Lut<R, N>& lut, float x_min, float x_max) noexcept {
  const auto dx = (x_max - x_min) / static_cast<float>(N - 1);
  auto       y  = lut.data();

  for (std::size_t i = 0; i < N; ++i) {
    if constexpr (std::is_same_v<R, float>) {
      y[i] = arm_sin_f32(x_min + static_cast<float>(i) * dx);
    } else {
      std::unreachable();
    }
  }
}

}   // namespace math