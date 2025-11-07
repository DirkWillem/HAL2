export module math:geometry.coordinate;

import :concepts;
import :settings;
import :functions.trigonometric;
import :functions.power;
import :linalg.vector;

namespace math {

export template <unsigned N = 2, concepts::Number R = float>
class CartesianCoordinate : public Vec<N, R> {
 public:
  constexpr CartesianCoordinate(R x, R y) noexcept
    requires(N == 2)
      : Vec<N, R>{x, y} {}

  constexpr CartesianCoordinate(R x, R y, R z) noexcept
    requires(N == 3)
      : Vec<N, R>{x, y, z} {}

  using Vec<N, R>::x;
  using Vec<N, R>::y;
  using Vec<N, R>::z;
};

/**
 * @brief Represents a coordinate in a polar coordinate system identified by a
 * radial and angular component (\c r, \c theta).
 * @tparam R Numeric type
 */
export template <concepts::Number R = float>
class PolarCoordinate : public Vec<2, R> {
 public:
  /**
   * @brief Returns the radial (\c r) component of the vector.
   *
   * @return Radial component.
   */
  [[nodiscard]] constexpr R r() const noexcept { return Vec<2, R>::vals[0]; }

  /**
   * @brief Returns the angular (\c theta) component of the vector.
   *
   * @return Angular component.
   */
  [[nodiscard]] constexpr R theta() const noexcept {
    return Vec<2, R>::vals[1];
  }
};

/**
 * @brief Converts a cartesian coordinate to polar coordinates given a center
 * point.
 * @tparam Num Numeric type.
 * @tparam S Function evaluation settings.
 * @param center Center point in cartesian coordinates.
 * @param x Point to convert in cartesian coordinates.
 * @param result \c PolarCoordinate to store the result in.
 */
export template <concepts::Number Num, Settings S = {}>
void CartesianToPolar(const CartesianCoordinate<2, Num>& center,
                      const CartesianCoordinate<2, Num>& x,
                      PolarCoordinate<Num>&              result) noexcept {
  const auto dx = x.x() - center.x();
  const auto dy = x.y() - center.y();

  result.vals[0] = Sqrt((dx * dx) + (dy * dy));
  result.vals[1] = Atan2<Num, S>(dy, dx);
}

}   // namespace math