#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <ranges>

#ifdef HAS_CMSIS_DSP

extern "C" {
#include <arm_math.h>
}
#endif

#include <math/concepts.h>
#include <math/functions/transcendental.h>

namespace math {

template <std::size_t N, Real R = float>
/**
 * Implementation of a column vector
 * @tparam N Vector dimension
 * @tparam F Numeric type
 */
struct Vec {
  constexpr Vec() noexcept                      = default;
  constexpr Vec(const Vec&) noexcept            = default;
  constexpr Vec(Vec&&) noexcept                 = default;
  constexpr Vec& operator=(const Vec&) noexcept = default;
  constexpr Vec& operator=(Vec&&) noexcept      = default;
  constexpr ~Vec() noexcept                     = default;

  /**
   * Constructor for a 2D vector
   * @param x X-component
   * @param y Y-component
   */
  constexpr Vec(R x, R y) noexcept
    requires(N == 2)
      : vals{{x, y}} {}

  /**
   * Constructor for a 3D vector
   * @param x X-component
   * @param y Y-component
   * @param z Z-component
   */
  constexpr Vec(R x, R y, R z) noexcept
    requires(N == 3)
      : vals{{x, y, z}} {}

  /**
   * General constructor
   * @param vals Vector values
   */
  explicit constexpr Vec(std::array<R, N> vals)
      : vals{vals} {}

  /**
   * Compound assignment operator
   * @param rhs Right-hand side
   * @return Current instance
   */
  constexpr Vec<N, R>& operator+=(const Vec<N, R>& rhs) noexcept {
    for (auto i = 0; i < N; i++) {
      vals[i] += rhs.vals[i];
    }

    return *this;
  }

  /**
   * Addition operator
   * @param rhs Right-hand side of the sum
   * @return Sum of the two vectors
   */
  constexpr Vec<N, R> operator+(const Vec<N, R>& rhs) const noexcept {
    auto result = *this;
    result += rhs;
    return result;
  }

  /**
   * Compound subtraction operator
   * @param rhs Right-hand side of the operation
   * @return Current instance
   */
  constexpr Vec<N, R>& operator-=(const Vec<N, R>& rhs) noexcept {
    for (auto i = 0; i < N; i++) {
      vals[i] -= rhs.vals[i];
    }

    return *this;
  }

  /**
   * Subtraction operator
   * @param rhs Right-hand side of the operation
   * @return Difference between two vectors
   */
  constexpr Vec<N, R> operator-(const Vec<N, R>& rhs) const noexcept {
    auto result = *this;
    result -= rhs;
    return result;
  }

  /**
   * Negation operator
   * @return Negated vector
   */
  constexpr Vec<N, R> operator-() const noexcept {
    std::array<R, N> result{};
    for (auto i = 0; i < N; i++) {
      result[i] = -vals[i];
    }
    return Vec<N, R>{result};
  }

  /**
   * Compound scalar multiplication operator
   * @param rhs Right-hand side of the operation
   * @return Current instance
   */
  constexpr Vec<N, R>& operator*=(R rhs) noexcept {
    for (auto i = 0; i < N; i++) {
      vals[i] *= rhs;
    }

    return *this;
  }

  /**
   * Scalar multiplication operator
   * @param rhs Right-hand side of the operation
   * @return Vector multiplied by scalar
   */
  constexpr Vec<N, R> operator*(R rhs) const noexcept {
    auto result = *this;
    result *= rhs;
    return result;
  }

  /**
   * Compound scalar division operator
   * @param rhs Right-hand side of the operation
   * @return Current instance
   */
  constexpr Vec<N, R>& operator/=(R rhs) noexcept {
    for (auto i = 0; i < N; i++) {
      vals[i] *= rhs;
    }

    return *this;
  }

  /**
   * Scalar division operator
   * @param rhs Right-hand side of the operation
   * @return Vector divided by scalar
   */
  constexpr Vec<N, R> operator/(R rhs) const noexcept {
    auto result = *this;
    result *= rhs;
    return *this;
  }

  template <FuncSettings S = FuncSettings{}>
  /**
   * Computes the magnitude of the vector
   * @tparam S Function settings to use when computing the magnitude
   * @return Vector magnitude
   */
  constexpr R Magnitude() const noexcept {
    R sum_of_squares{};
    for (auto x : vals) {
      sum_of_squares += x * x;
    }
    return Sqrt<R, S>(sum_of_squares);
  }

  template <FuncSettings S = FuncSettings{}>
  /**
   * Returns the normalized version of the current vector, i.e. a vector
   *   in the same direction with magnitude 1
   * @tparam S Function settings
   * @return Normalized vector
   */
  constexpr Vec<N, R> Normalized() const noexcept {
    const auto mag = Magnitude<S>();

    std::array<R, N> normalized_vals{};
    std::ranges::transform(vals, normalized_vals.begin(),
                           [mag](const auto x) { return x / mag; });
    return Vec<N, R>{normalized_vals};
  }

  /**
   * Dot product operator
   * @param rhs Right-hand side of the operation
   * @return Dot product of two vectors
   */
  constexpr R Dot(const Vec<N, R>& rhs) const noexcept {
    R result{};

    for (auto i = 0; i < N; i++) {
      result += vals[i] * rhs.vals[i];
    }

    return result;
  }

  /**
   * Cross product operator
   * @param rhs Right-hand side of the operation
   * @return Cross product
   */
  constexpr Vec<3, R> Cross(const Vec<N, R>& rhs) const noexcept
    requires(N == 3)
  {
    return {
        y() * rhs.z() - z() * rhs.y(),
        z() * rhs.x() - x() * rhs.z(),
        x() * rhs.y() - y() * rhs.x(),
    };
  }

  /**
   * Returns the x-component of the vector in case of a 2D or 3D vector
   * @return X-component
   */
  [[nodiscard]] constexpr R x() const noexcept
    requires(N == 2 || N == 3)
  {
    return vals[0];
  }

  /**
   * Returns the x-component of the vector in case of a 2D or 3D vector
   * @return X-component
   */
  [[nodiscard]] constexpr R y() const noexcept
    requires(N == 2 || N == 3)
  {
    return vals[1];
  }

  /**
   * Returns the x-component of the vector in case of a 2D or 3D vector
   * @return X-component
   */
  [[nodiscard]] constexpr R z() const noexcept
    requires(N == 3)
  {
    return vals[2];
  }

  std::array<R, N> vals{};
};

template <std::size_t N, Real R>
constexpr Vec<N, R> operator*(R lhs, const Vec<N, R>& rhs) noexcept {
  return rhs * lhs;
}

}   // namespace math