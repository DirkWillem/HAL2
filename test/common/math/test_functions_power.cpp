#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>

#include <math/functions/transcendental.h>

using namespace ::testing;

TEST(MathFuncsPower, SqrtUsingNewtonRaphson) {
  constexpr auto FS = math::FuncSettings{
      .implementation = math::Implementation::ForceNewtonRaphsonApproximation,
      .newton_raphson_iterations = 16, // Adjust as necessary
  };

  // Test known values for perfect squares
  ASSERT_THAT((math::Sqrt<float, FS>(0.F)), FloatNear(0.F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(1.F)), FloatNear(1.F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(4.F)), FloatNear(2.F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(9.F)), FloatNear(3.F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(16.F)), FloatNear(4.F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(25.F)), FloatNear(5.F, 1e-6F));


  // Test values that are not perfect squares
  ASSERT_THAT((math::Sqrt<float, FS>(2.F)), FloatNear(1.414213F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(3.F)), FloatNear(1.732050F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(5.F)), FloatNear(2.236067F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(10.F)), FloatNear(3.162278F, 1e-6F));

  // Test small positive values close to zero for precision
  ASSERT_THAT((math::Sqrt<float, FS>(1e-6F)), FloatNear(1e-3F, 1e-6F));
  ASSERT_THAT((math::Sqrt<float, FS>(1e-4F)), FloatNear(1e-2F, 1e-6F));

  // Test large values for potential performance issues or precision loss
  ASSERT_THAT((math::Sqrt<float, FS>(1e6F)), FloatNear(1e3F, 1e-3F));
  ASSERT_THAT((math::Sqrt<float, FS>(1e8F)), FloatNear(1e4F, 1e-3F));

  // Edge case: Test zero
  ASSERT_THAT((math::Sqrt<float, FS>(0.F)), FloatNear(0.F, 1e-6F));

  // Edge case: Test negative values, expect NaN or some error handling
  ASSERT_TRUE(std::isnan(math::Sqrt<float, FS>(-1.F)));
  ASSERT_TRUE(std::isnan(math::Sqrt<float, FS>(-100.F)));
}