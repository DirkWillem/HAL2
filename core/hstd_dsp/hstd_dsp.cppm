module;

#include <array>
#include <chrono>
#include <concepts>

export module hstd.dsp;

import hstd;
import math;

namespace hstd::dsp {

/**
 * @brief Calculates a filter sample count to approximate a moving average over
 * the given time window.
 * @param t_window Desired window time.
 * @param fs Sampling frequency.
 * @return Filter sample count.
 */
export constexpr std::size_t CalculateFilterSize(Duration auto  t_window,
                                                 Frequency auto fs) {
  const float fs_hz = static_cast<float>(fs.template As<Hz>().count);
  const float t_window_s =
      static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                             hstd::MakeDuration(t_window))
                             .count())
      / 1000.F;

  return math::FastCeil<std::size_t>(t_window_s * fs_hz);
}

/**
 * @brief Filter that implements an exponentially weighted moving average
 * (EWMA) by approxomating an N-sample moving average.
 */
export template <std::floating_point F>
class ExponentialMovingAverage {
 public:
  /**
   * @brief Constructor.
   * @param n Number of samples to approximate.
   */
  constexpr explicit ExponentialMovingAverage(std::size_t n) noexcept
      : n{n}
      , state{}
      , alpha{static_cast<F>(n - 1) / static_cast<F>(n)}
      , one_over_n{static_cast<F>(1) / static_cast<F>(n)} {}

  /**
   * @brief Resets the filter.
   */
  constexpr void Reset() noexcept {
    n_samples = 0;
    state     = F{};
  }

  /**
   * @brief Updates the filter and returns the current filtered value.
   * @param x Sample to update the filter with.
   * @return New filtered value.
   */
  constexpr F Update(F x) noexcept {
    if (n_samples < n) {
      n_samples++;
      state += x * one_over_n;
    } else {
      state = (alpha * state) + (one_over_n * x);
    }

    return state;
  }

  /**
   * @brief Initializes the filter to the given value.
   * @param value Value to initialize the filter to.
   * @return Current filter value.
   */
  constexpr F Initialize(F value) {
    state     = value;
    n_samples = n;
    return state;
  }

  /**
   * @brief Returns whether the filter is initialized.
   * @return Whether the filter is initialized.
   */
  [[nodiscard]] constexpr bool Initialized() const noexcept {
    return n == n_samples;
  }

  /**
   * @brief Returns the current value of the filter.
   * @return Current value of the filter.
   */
  [[nodiscard]] constexpr F value() const noexcept { return state; }

 private:
  std::size_t n_samples{0};
  std::size_t n;
  F           state;
  F           alpha;
  F           one_over_n;
};
}   // namespace hstd::dsp