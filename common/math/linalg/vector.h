#pragma once

#include <array>
#include <concepts>

#ifdef HAS_CMSIS_DSP

extern "C" {
#include <arm_math.h>
}
#endif

namespace math {

template <std::size_t N, std::floating_point F = float>
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
  constexpr Vec(F x, F y) noexcept
    requires(N == 2)
      : vals{{x, y}} {}

  /**
   * Constructor for a 3D vector
   * @param x X-component
   * @param y Y-component
   * @param z Z-component
   */
  constexpr Vec(F x, F y, F z) noexcept
    requires(N == 3)
      : vals{{x, y, z}} {}

  /**
   * General constructor
   * @param vals Vector values
   */
  explicit constexpr Vec(std::array<F, N> vals)
      : vals{vals} {}

  /**
   * Compound assignment operator
   * @param rhs Right-hand side
   * @return Current instance
   */
  constexpr Vec<N, F>& operator+=(const Vec<N, F>& rhs) noexcept {
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
  constexpr Vec<N, F> operator+(const Vec<N, F>& rhs) const noexcept {
    auto result = *this;
    result += rhs;
    return result;
  }

  /**
   * Compound subtraction operator
   * @param rhs Right-hand side of the operation
   * @return Current instance
   */
  constexpr Vec<N, F>& operator-=(const Vec<N, F>& rhs) noexcept {
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
  constexpr Vec<N, F> operator-(const Vec<N, F>& rhs) const noexcept {
    auto result = *this;
    result -= rhs;
    return result;
  }

  /**
   * Negation operator
   * @return Negated vector
   */
  constexpr Vec<N, F> operator-() const noexcept {
    std::array<F, N> result{};
    for (auto i = 0; i < N; i++) {
      result[i] = -vals[i];
    }
    return Vec<N, F>{result};
  }

  /**
   * Compound scalar multiplication operator
   * @param rhs Right-hand side of the operation
   * @return Current instance
   */
  constexpr Vec<N, F>& operator*=(F rhs) noexcept {
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
  constexpr Vec<N, F> operator*(F rhs) const noexcept {
    auto result = *this;
    result *= rhs;
    return *this;
  }

  /**
   * Compound scalar division operator
   * @param rhs Right-hand side of the operation
   * @return Current instance
   */
  constexpr Vec<N, F>& operator/=(F rhs) noexcept {
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
  constexpr Vec<N, F> operator/(F rhs) const noexcept {
    auto result = *this;
    result *= rhs;
    return *this;
  }

  /**
   * Dot product operator
   * @param rhs Right-hand side of the operation
   * @return Dot product of two vectors
   */
  constexpr F Dot(const Vec<N, F>& rhs) noexcept {
    F result{};

    for (auto i = 0; i < N; i++) {
      result += vals[i] * rhs.vals[i];
    }

    return result;
  }

  /**
   * Returns the x-component of the vector in case of a 2D or 3D vector
   * @return X-component
   */
  [[nodiscard]] constexpr float x() const noexcept
    requires(N == 2 || N == 3)
  {
    return vals[0];
  }

  /**
   * Returns the x-component of the vector in case of a 2D or 3D vector
   * @return X-component
   */
  [[nodiscard]] constexpr float y() const noexcept
    requires(N == 2 || N == 3)
  {
    return vals[1];
  }

  /**
   * Returns the x-component of the vector in case of a 2D or 3D vector
   * @return X-component
   */
  [[nodiscard]] constexpr float z() const noexcept
    requires(N == 3)
  {
    return vals[2];
  }

  std::array<F, N> vals{};
};

template <std::size_t N, std::floating_point F>
constexpr Vec<N, F> operator*(F lhs, const Vec<N, F>& rhs) noexcept {
  return rhs * lhs;
}

template <std::size_t N, std::floating_point F>
constexpr Vec<N, F> operator/(F lhs, const Vec<N, F>& rhs) noexcept {
  return rhs / lhs;
}

}   // namespace math