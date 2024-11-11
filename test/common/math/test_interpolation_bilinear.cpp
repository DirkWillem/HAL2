#include <gtest/gtest.h>

#include <math/functions/interpolation.h>

#include <fp/fix.h>

TEST(MathInterpBilinear, FloatingPointInterpolationAtCorners) {
  constexpr double y00 = 0.0;
  constexpr double y10 = 1.0;
  constexpr double y01 = 2.0;
  constexpr double y11 = 3.0;

  EXPECT_NEAR(y00,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.0, 0.0),
              1e-6);
  EXPECT_NEAR(y10,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 1.0, 0.0),
              1e-6);
  EXPECT_NEAR(y01,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.0, 1.0),
              1e-6);
  EXPECT_NEAR(y11,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 1.0, 1.0),
              1e-6);
}

TEST(MathInterpBilinear, FloatingPointInterpolationAtCenter) {
  constexpr double y00 = 0.0;
  constexpr double y10 = 1.0;
  constexpr double y01 = 2.0;
  constexpr double y11 = 3.0;

  constexpr double expected_center_value = (y00 + y10 + y01 + y11) / 4.0;
  EXPECT_NEAR(expected_center_value,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.5, 0.5),
              1e-6);
}

TEST(MathInterpBilinear, FloatingPointInterpolationAlongEdges) {
  constexpr double y00 = 0.0;
  constexpr double y10 = 1.0;
  constexpr double y01 = 2.0;
  constexpr double y11 = 3.0;

  EXPECT_NEAR(0.5,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.5, 0.0),
              1e-6);
  EXPECT_NEAR(2.5,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.5, 1.0),
              1e-6);
  EXPECT_NEAR(1.0,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.0, 0.5),
              1e-6);
  EXPECT_NEAR(2.0,
              math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 1.0, 0.5),
              1e-6);
}

TEST(MathInterpBilinear, FloatingPointInterpolationWithinInterior) {
  constexpr double y00 = 0.0;
  constexpr double y10 = 4.0;
  constexpr double y01 = 2.0;
  constexpr double y11 = 6.0;

  constexpr double expected_value = 2.5;
  EXPECT_NEAR(
      expected_value,
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, 0.25, 0.75),
      1e-6);
}

using UQ16_16 = fp::UQ<16, 16>;

TEST(MathInterpBilinear, FixedPointInterpolationAtCorners) {
  constexpr UQ16_16 y00{0.0};
  constexpr UQ16_16 y10{1.0};
  constexpr UQ16_16 y01{2.0};
  constexpr UQ16_16 y11{3.0};

  constexpr UQ16_16 zero{0};
  constexpr UQ16_16 one{1};

  const auto r00 =
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, zero, zero);
  const auto r10 =
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, one, zero);
  const auto r01 =
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, zero, one);
  const auto r11 =
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, one, one);

  ASSERT_EQ(r00.raw(), y00.raw());
  ASSERT_EQ(r10.raw(), y10.raw());
  ASSERT_EQ(r01.raw(), y01.raw());
  ASSERT_EQ(r11.raw(), y11.raw());
}

TEST(MathInterpBilinear, FixedPointInterpolationAtCenter) {
  constexpr UQ16_16 y00{0.0};
  constexpr UQ16_16 y10{1.0};
  constexpr UQ16_16 y01{2.0};
  constexpr UQ16_16 y11{3.0};

  constexpr UQ16_16 half{0.5};
  constexpr UQ16_16 expected{(1.0 + 2.0 + 3.0) / 4.0};

  EXPECT_EQ(expected.raw(),
            math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, half, half)
                .raw());
}
//
TEST(MathInterpBilinear, FixedPointInterpolationAtEdges) {
  constexpr UQ16_16 y00{0};
  constexpr UQ16_16 y10{1};
  constexpr UQ16_16 y01{2};
  constexpr UQ16_16 y11{3};

  constexpr UQ16_16 zero{0};
  constexpr UQ16_16 half{0.5};
  constexpr UQ16_16 one{1};

  EXPECT_EQ(UQ16_16{0.5}.raw(),
            math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, half, zero)
                .raw());
  EXPECT_EQ(
      UQ16_16{2.5}.raw(),
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, half, one).raw());
  EXPECT_EQ(UQ16_16{1.0}.raw(),
            math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, zero, half)
                .raw());
  EXPECT_EQ(
      UQ16_16{2.0}.raw(),
      math::BilinearInterpolateUnitSquare(y00, y10, y01, y11, one, half).raw());
}

TEST(MathInterpBilinear, FixedPointInterpolationWithinInterior) {
  constexpr UQ16_16 y00{0};
  constexpr UQ16_16 y10{4};
  constexpr UQ16_16 y01{2};
  constexpr UQ16_16 y11{6};

  constexpr UQ16_16 expected{2.5};

  EXPECT_EQ(expected.raw(),
            math::BilinearInterpolateUnitSquare(y00, y10, y01, y11,
                                                UQ16_16{0.25}, UQ16_16{0.75})
                .raw());
}