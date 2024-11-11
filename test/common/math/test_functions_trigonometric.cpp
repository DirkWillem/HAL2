#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>

#include <math/functions/transcendental.h>

using namespace ::testing;

TEST(MathFuncsTrig, FloatingPointSinUsingTaylor) {
  constexpr auto FS = math::FuncSettings{
      .implementation = math::Implementation::ForceTaylorSeriesApproximation,
      .taylor_series_order = 9,
  };

  // Test known values at standard angles
  ASSERT_THAT((math::Sin<float, FS>(0.F)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(0.5F * M_PI)), FloatNear(1.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(M_PI)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(1.5F * M_PI)), FloatNear(-1.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(2.0F * M_PI)), FloatNear(0.F, 1e-6F));

  // Test at some fractions of π for better coverage
  ASSERT_THAT((math::Sin<float, FS>(M_PI / 6)), FloatNear(0.5F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(M_PI / 4)),
              FloatNear(std::sqrt(2) / 2, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(M_PI / 3)),
              FloatNear(std::sqrt(3) / 2, 1e-6F));

  // Test some negative values
  ASSERT_THAT((math::Sin<float, FS>(-0.5F * M_PI)), FloatNear(-1.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(-M_PI)), FloatNear(0.F, 1e-6F));

  // Test values beyond 2π (tests periodicity)
  ASSERT_THAT((math::Sin<float, FS>(2.5F * M_PI)), FloatNear(1.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(3.0F * M_PI)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(4.0F * M_PI)), FloatNear(0.F, 1e-6F));

  // Test very small angles close to zero to check precision for small inputs
  ASSERT_THAT((math::Sin<float, FS>(1e-5F)), FloatNear(1e-5F, 1e-6F));
  ASSERT_THAT((math::Sin<float, FS>(-1e-5F)), FloatNear(-1e-5F, 1e-6F));
}

TEST(MathFuncsTrig, FixedPointSinUsingTaylor) {
  using Q16_16 = fp::Q<16, 16>;

  constexpr auto FS = math::FuncSettings{
      .implementation = math::Implementation::ForceTaylorSeriesApproximation,
      .taylor_series_order = 10,
  };

  // Test known values at standard angles
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{0.0})),
              FloatNear(0.F, 1e-6F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{0.5F * M_PI})),
              FloatNear(1.F, 1e-6F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{M_PI})),
              FloatNear(0.F, 1e-6F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{1.5F * M_PI})),
              FloatNear(-1.F, 1e-6F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{2.0F * M_PI})),
              FloatNear(0.F, 1e-6F));

  // Test at some fractions of π for better coverage
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{M_PI / 6})),
              FloatNear(0.5F, 1e-5F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{M_PI / 4})),
              FloatNear(std::sqrt(2) / 2, 1e-5F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{M_PI / 3})),
              FloatNear(std::sqrt(3) / 2, 1e-5F));

  // Test some negative values
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{-0.5F * M_PI})),
              FloatNear(-1.F, 1e-5F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{-M_PI})),
              FloatNear(0.F, 1e-5F));

  // Test values beyond 2π (tests periodicity)
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{2.5F * M_PI})),
              FloatNear(1.F, 1e-5F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{3.0F * M_PI})),
              FloatNear(0.F, 1e-5F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{4.0F * M_PI})),
              FloatNear(0.F, 1e-5F));

  // Test very small angles close to zero to check precision for small
  // inputs
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{1e-5F})),
              FloatNear(1e-5F, 1e-5F));
  ASSERT_THAT(static_cast<float>(math::Sin<Q16_16, FS>(Q16_16{-1e-5F})),
              FloatNear(-1e-5F, 1e-5F));
}

TEST(MathFuncsTrig, CosUsingTaylor) {
  constexpr auto FS = math::FuncSettings{
      .implementation = math::Implementation::ForceTaylorSeriesApproximation,
      .taylor_series_order = 9,
  };

  // Test known values at standard angles
  ASSERT_THAT((math::Cos<float, FS>(0.F)), FloatNear(1.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(0.5F * M_PI)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(M_PI)), FloatNear(-1.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(1.5F * M_PI)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(2.0F * M_PI)), FloatNear(1.F, 1e-6F));

  // Test at some fractions of π for better coverage
  ASSERT_THAT((math::Cos<float, FS>(M_PI / 6)),
              FloatNear(std::sqrt(3) / 2, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(M_PI / 4)),
              FloatNear(std::sqrt(2) / 2, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(M_PI / 3)), FloatNear(0.5F, 1e-6F));

  // Test some negative values
  ASSERT_THAT((math::Cos<float, FS>(-0.5F * M_PI)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(-M_PI)), FloatNear(-1.F, 1e-6F));

  // Test values beyond 2π (tests periodicity)
  ASSERT_THAT((math::Cos<float, FS>(2.5F * M_PI)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(3.0F * M_PI)), FloatNear(-1.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(4.0F * M_PI)), FloatNear(1.F, 1e-6F));

  // Test very small angles close to zero to check precision for small inputs
  ASSERT_THAT((math::Cos<float, FS>(1e-5F)), FloatNear(1.F, 1e-6F));
  ASSERT_THAT((math::Cos<float, FS>(-1e-5F)), FloatNear(1.F, 1e-6F));
}
