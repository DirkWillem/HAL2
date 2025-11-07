module;

#include <algorithm>
#include <array>
#include <concepts>
#include <ranges>

#ifdef HAS_CMSIS_DSP

extern "C" {
#include <arm_math.h>
}
#endif

export module math:linalg.vector;

import :concepts;

namespace math {

/**
 * @brief Implementation of a column vector.
 *
 * @tparam N Vector dimension.
 * @tparam R Numeric type.
 */
export template <std::size_t N, concepts::Number R = float>
struct Vec {
  constexpr Vec() noexcept                      = default;
  constexpr Vec(const Vec&) noexcept            = default;
  constexpr Vec(Vec&&) noexcept                 = default;
  constexpr Vec& operator=(const Vec&) noexcept = default;
  constexpr Vec& operator=(Vec&&) noexcept      = default;
  constexpr ~Vec() noexcept                     = default;

  /**
   * @brief Constructor for a 2D vector.
   *
   * @param x X-component.
   * @param y Y-component.
   */
  constexpr Vec(R x, R y) noexcept
    requires(N == 2)
      : vals{{x, y}} {}

  /**
   * @brief Constructor for a 3D vector.
   *
   * @param x X-component.
   * @param y Y-component.
   * @param z Z-component.
   */
  constexpr Vec(R x, R y, R z) noexcept
    requires(N == 3)
      : vals{{x, y, z}} {}

  /**
   * @brief General constructor.
   *
   * @param vals Vector values.
   */
  explicit constexpr Vec(std::array<R, N> vals)
      : vals{vals} {}

  /**
   * @brief Compound assignment operator.
   *
   * @param rhs Right-hand side.
   * @return Current instance.
   */
  constexpr Vec<N, R>& operator+=(const Vec<N, R>& rhs) noexcept {
    // Unrolled loops for N=2, 3
    if constexpr (N == 2) {
      vals[0] += rhs.vals[0];
      vals[1] += rhs.vals[1];
    } else if constexpr (N == 3) {
      vals[0] += rhs.vals[0];
      vals[1] += rhs.vals[1];
      vals[2] += rhs.vals[2];
    } else {
      for (auto i = 0; i < N; i++) {
        vals[i] += rhs.vals[i];
      }
    }

    return *this;
  }

  /**
   * @brief Addition operator.
   *
   * @param rhs Right-hand side of the sum.
   * @return Sum of the two vectors.
   */
  constexpr Vec<N, R> operator+(const Vec<N, R>& rhs) const noexcept {
    auto result = *this;
    result += rhs;
    return result;
  }

  /**
   * @brief Compound subtraction operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Current instance.
   */
  constexpr Vec<N, R>& operator-=(const Vec<N, R>& rhs) noexcept {
    // Unrolled loops for N=2, 3
    if constexpr (N == 2) {
      vals[0] -= rhs.vals[0];
      vals[1] -= rhs.vals[1];
    } else if constexpr (N == 3) {
      vals[0] -= rhs.vals[0];
      vals[1] -= rhs.vals[1];
      vals[2] -= rhs.vals[2];
    } else {
      for (auto i = 0; i < N; i++) {
        vals[i] -= rhs.vals[i];
      }
    }

    return *this;
  }

  /**
   * @brief Subtraction operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Difference between two vectors.
   */
  constexpr Vec<N, R> operator-(const Vec<N, R>& rhs) const noexcept {
    auto result = *this;
    result -= rhs;
    return result;
  }

  /**
   * @brief Negation operator.
   *
   * @return Negated vector.
   */
  constexpr Vec<N, R> operator-() const noexcept {
    std::array<R, N> result{};
    for (auto i = 0; i < N; i++) {
      result[i] = -vals[i];
    }
    return Vec<N, R>{result};
  }

  /**
   * @brief Compound scalar multiplication operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Current instance.
   */
  constexpr Vec<N, R>& operator*=(R rhs) noexcept {
    // Unrolled loops for N=2, 3
    if constexpr (N == 2) {
      vals[0] *= rhs;
      vals[1] *= rhs;
    } else if constexpr (N == 3) {
      vals[0] *= rhs;
      vals[1] *= rhs;
      vals[2] *= rhs;
    } else {
      for (auto i = 0; i < N; i++) {
        vals[i] *= rhs;
      }
    }

    return *this;
  }

  /**
   * @brief Scalar multiplication operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Vector multiplied by scalar.
   */
  constexpr Vec<N, R> operator*(R rhs) const noexcept {
    auto result = *this;
    result *= rhs;
    return result;
  }

  /**
   * @brief Compound scalar division operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Current instance.
   */
  constexpr Vec<N, R>& operator/=(R rhs) noexcept {
    // Unrolled loops for N=2, 3
    if constexpr (N == 2) {
      vals[0] /= rhs;
      vals[1] /= rhs;
    } else if constexpr (N == 3) {
      vals[0] /= rhs;
      vals[1] /= rhs;
      vals[2] /= rhs;
    } else {
      for (auto i = 0; i < N; i++) {
        vals[i] /= rhs;
      }
    }

    return *this;
  }

  /**
   * @brief Scalar division operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Vector divided by scalar.
   */
  constexpr Vec<N, R> operator/(R rhs) const noexcept {
    auto result = *this;
    result /= rhs;
    return result;
  }

  /**
   * @brief Dot product operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Dot product of two vectors.
   */
  constexpr R Dot(const Vec<N, R>& rhs) const noexcept {
    R result{};

    for (auto i = 0; i < N; i++) {
      result += vals[i] * rhs.vals[i];
    }

    return result;
  }

  /**
   * @brief Cross product operator.
   *
   * @param rhs Right-hand side of the operation.
   * @return Cross product.
   */
  constexpr Vec<3, R> Cross(const Vec<N, R>& rhs) const noexcept
    requires(N == 3)
  {
    // ReSharper disable CppRedundantParentheses
    return {
        (y() * rhs.z()) - (z() * rhs.y()),
        (z() * rhs.x()) - (x() * rhs.z()),
        (x() * rhs.y()) - (y() * rhs.x()),
    };
    // ReSharper restore CppRedundantParentheses
  }

  std::array<R, N> vals{};

 protected:
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
   * @brief Returns the y-component of the vector in case of a 2D or 3D vector.
   *
   * @return Y-component.
   */
  [[nodiscard]] constexpr R y() const noexcept
    requires(N == 2 || N == 3)
  {
    return vals[1];
  }

  /**
   * @brief Returns the z-component of the vector in case of a 3D vector.
   *
   * @return Z-component.
   */
  [[nodiscard]] constexpr R z() const noexcept
    requires(N == 3)
  {
    return vals[2];
  }
};

template <std::size_t N, concepts::Number R>
constexpr Vec<N, R> operator*(R lhs, const Vec<N, R>& rhs) noexcept {
  return rhs * lhs;
}

}   // namespace math