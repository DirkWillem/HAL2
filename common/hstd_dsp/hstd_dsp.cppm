module;

#include <array>
#include <concepts>

export module hstd.dsp;

import hstd;

namespace hstd::dsp {

/**
 * @brief IIR filter that approximates an N-point moving average filter.
 */
export template <std::floating_point F>
class MovingAverageIir {
 public:
  /**
   * @brief Constructor.
   * @param n Number of samples to approximate.
   */
  constexpr explicit MovingAverageIir(std::size_t n) noexcept
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
   * @brief Returns whether the filter is initialized.
   * @return Whether the filter is initialized.
   */
  [[nodiscard]] constexpr bool Initialized() const noexcept {
    return n == n_samples;
  }

 private:
  std::size_t n_samples{0};
  std::size_t n;
  F           state;
  F           alpha;
  F           one_over_n;
};

}   // namespace hstd::dsp