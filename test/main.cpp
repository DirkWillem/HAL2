#include <math/geometry/transformation_matrix.h>
#include <math/linalg/matrix.h>

#include <iostream>

int main() {
  constexpr auto PlaneTo3D = math::Transform2DTo3DMatrix(
      math::Vec<3, float>{0.F, 1.F, 0.F}, math::Vec<3, float>{0.F, 0.F, 1.F});

  constexpr auto Pos   = math::Vec<2, float>{2.F, 3.F};
  constexpr auto Pos3D = PlaneTo3D * Pos;
  constexpr auto Pos3D_2 =
      math::Transform2DTo3D(Pos, math::Vec<3, float>{0.F, 1.F, 0.F},
                            math::Vec<3, float>{0.F, 0.F, 1.F});

  constexpr math::Vec<3, float> x{1.F, 1.F, 0.F};

  constexpr auto rot2 =
      math::RotationMatrix3D(math::RotateZ{static_cast<float>(M_PI)});
  constexpr auto rot = math::RotationMatrix3D(
      math::RotateEuler{0.F, 0.F, static_cast<float>(M_PI)});

  constexpr auto x2 = rot * x;

  int a = 65;
}
