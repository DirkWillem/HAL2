import math;

#include <cmath>

#include <gtest/gtest.h>

using namespace math;

TEST(Coordinate, CartesianToPolar) {
  PolarCoordinate result{};

  CartesianToPolar({10.F, 10.F}, {20.F, 10.F}, result);
  ASSERT_FLOAT_EQ(result.r(), 10.F);
  ASSERT_FLOAT_EQ(result.theta(), 0.F);

  CartesianToPolar({10.F, 10.F}, {10.F, 20}, result);
  ASSERT_FLOAT_EQ(result.r(), 10.F);
  ASSERT_FLOAT_EQ(result.theta(), 0.5F * M_PI);

  CartesianToPolar({10.F, 10.F}, {0.F, 0.F}, result);
  ASSERT_FLOAT_EQ(result.r(), 14.1421356F);   // sqrt(10^2 + 10^2)
  ASSERT_FLOAT_EQ(result.theta(), -0.75F * M_PI);
}