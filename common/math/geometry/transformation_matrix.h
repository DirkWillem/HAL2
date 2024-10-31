#include <concepts>

#include <math/functions/transcendental.h>
#include <math/linalg/matrix.h>

namespace math {

template <std::floating_point F>
struct RotateX {
  using NumType = F;

  F rx;
};

template <std::floating_point F>
struct RotateY {
  using NumType = F;

  F ry;
};

template <std::floating_point F>
struct RotateZ {
  using NumType = F;

  F rz;
};

template <std::floating_point F>
struct RotateEuler {
  using NumType = F;

  F alpha;
  F beta;
  F gamma;
};

template <std::floating_point F>
struct RotateYawPitchRoll {
  using NumType = F;

  F yaw;
  F pitch;
  F roll;
};

template <typename T>
struct Is2DRotationHelper : std::false_type {};

template <std::floating_point F>
struct Is2DRotationHelper<RotateZ<F>> : std::true_type {};

template <typename T>
struct Is3DRotationHelper : std::false_type {};

template <std::floating_point F>
struct Is3DRotationHelper<RotateX<F>> : std::true_type {};

template <std::floating_point F>
struct Is3DRotationHelper<RotateY<F>> : std::true_type {};

template <std::floating_point F>
struct Is3DRotationHelper<RotateZ<F>> : std::true_type {};

template <std::floating_point F>
struct Is3DRotationHelper<RotateEuler<F>> : std::true_type {};

template <std::floating_point F>
struct Is3DRotationHelper<RotateYawPitchRoll<F>> : std::true_type {};

template <typename T>
concept Is2DRotation = Is3DRotationHelper<T>::value;

template <typename T>
concept Is3DRotation = Is3DRotationHelper<T>::value;

template <Is2DRotation R, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<2, 2, typename R::NumType>
RotationMatrix2D(R rotation) noexcept {
  using F = typename R::NumType;
  if constexpr (std::is_same_v<R, RotateZ<F>>) {
    const auto [sin_theta, cos_theta] = SinCos<F, S>(rotation.rz);

    return Mat<2, 2, F>{{{
        {{cos_theta, -sin_theta}},
        {{sin_theta, cos_theta}},
    }}};
  } else {
    std::unreachable();
  }
}

namespace detail {

template <std::floating_point F, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<3, 3, F> RotateX3D(float theta) noexcept {
  constexpr F z0      = static_cast<F>(0.0);
  constexpr F one     = static_cast<F>(1.0);
  const auto [st, ct] = SinCos<F, S>(theta);

  return Mat<3, 3, F>{{{
      {{one, z0, z0}},
      {{z0, ct, -st}},
      {{z0, st, ct}},
  }}};
}

template <std::floating_point F, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<3, 3, F> RotateY3D(float theta) noexcept {
  constexpr F z0      = static_cast<F>(0.0);
  constexpr F one     = static_cast<F>(1.0);
  const auto [st, ct] = SinCos<F, S>(theta);

  return Mat<3, 3, F>{{{
      {{ct, z0, st}},
      {{z0, one, z0}},
      {{-st, z0, ct}},
  }}};
}

template <std::floating_point F, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<3, 3, F> RotateZ3D(float theta) noexcept {
  constexpr F z0      = static_cast<F>(0.0);
  constexpr F one     = static_cast<F>(1.0);
  const auto [st, ct] = SinCos<F, S>(theta);

  return Mat<3, 3, F>{{{
      {{ct, -st, z0}},
      {{st, ct, z0}},
      {{z0, z0, one}},
  }}};
}

template <std::floating_point F, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<3, 3, F> RotateEuler3D(float alpha, float beta,
                                                   float gamma) noexcept {
  const auto [sa, ca] = SinCos<F, S>(alpha);
  const auto [sb, cb] = SinCos<F, S>(beta);
  const auto [sg, cg] = SinCos<F, S>(gamma);

  return Mat<3, 3, F>{{{
      {{cb * cg, (sa * sb * cg) - (ca * sg), (ca * sb * cg) + (sa * sg)}},
      {{cb * sg, (sa * sb * sg) + (ca * cg), (ca * sb * sg) - (sa * cg)}},
      {{-sb, sa * cb, ca * cb}},
  }}};
}

template <std::floating_point F, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<3, 3, F> RotateYawPitchRoll(float alpha, float beta,
                                                        float gamma) noexcept {
  const auto [sa, ca] = SinCos<F, S>(alpha);
  const auto [sb, cb] = SinCos<F, S>(beta);
  const auto [sg, cg] = SinCos<F, S>(gamma);

  return Mat<3, 3, F>{{{
      {{ca * cb, (ca * sb * sg) - (sa * cg), (ca * sb * cg) + (sa * sg)}},
      {{sa * cb, (sa * sb * sg) - (ca * cg), (sa * sb * cg) - (ca * sg)}},
      {{-sb, cb * sg, cb * cg}},
  }}};
}

}   // namespace detail

template <Is3DRotation R, FuncSettings S = FuncSettings{}>
[[nodiscard]] constexpr Mat<3, 3, typename R::NumType>
RotationMatrix3D(R rotation) noexcept {
  using F = typename R::NumType;
  if constexpr (std::is_same_v<R, RotateX<F>>) {
    return detail::RotateX3D<F, S>(rotation.rx);
  } else if constexpr (std::is_same_v<R, RotateX<F>>) {
    return detail::RotateY3D<F, S>(rotation.ry);
  } else if constexpr (std::is_same_v<R, RotateZ<F>>) {
    return detail::RotateZ3D<F, S>(rotation.rz);
  } else if constexpr (std::is_same_v<R, RotateYawPitchRoll<F>>) {
    return detail::RotateYawPitchRoll<F, S>(rotation.yaw, rotation.pitch,
                                            rotation.roll);
  } else if constexpr (std::is_same_v<R, RotateEuler<F>>) {
    return detail::RotateEuler3D<F>(rotation.alpha, rotation.beta,
                                    rotation.gamma);
  } else {
    std::unreachable();
  }
}

template <std::floating_point F>
[[nodiscard]] constexpr Vec<3, F>
Transform2DTo3D(const Vec<2, F>& vec_2d, const Vec<3, F>& basis_x,
                const Vec<3, F> basis_y) noexcept {
  return Vec<3, F>{
      vec_2d.x() * basis_x.x() + vec_2d.y() * basis_y.x(),
      vec_2d.x() * basis_x.y() + vec_2d.y() * basis_y.y(),
      vec_2d.x() * basis_x.z() + vec_2d.y() * basis_y.z(),
  };
}

template <std::floating_point F>
[[nodiscard]] constexpr Mat<2, 3, F>
Transform2DTo3DMatrix(const Vec<3, F>& basis_x,
                      const Vec<3, F>& basis_y) noexcept {
  return Mat<2, 3, F>{{{
      {{basis_x.x(), basis_y.x()}},
      {{basis_x.y(), basis_y.y()}},
      {{basis_x.z(), basis_y.z()}},
  }}};
}

}   // namespace math