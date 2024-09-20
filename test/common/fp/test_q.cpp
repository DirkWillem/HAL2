#include <gtest/gtest.h>

#include <fp/fix.h>

using Q1_15 = fp::Q<1, 15>;
using Q1_31 = fp::Q<1, 31>;

using Q8_8 = fp::Q<8, 8>;

TEST(FP_Q, FromFloat) {
  ASSERT_EQ(Q1_15::Approximate(0.5F).raw(), Q1_15::Scale / 2);

  ASSERT_EQ(Q8_8::Approximate(4.F).raw(), Q8_8::Scale * 4);
  ASSERT_EQ(Q8_8::Approximate(0.25F).raw(), Q8_8::Scale / 4);

  ASSERT_EQ(Q8_8::Approximate(-12.F).raw(), -Q8_8::Scale * 12);
}

TEST(FP_Q, AdditionOfSameType) {
  const auto a = Q1_15::Approximate(0.25F);
  const auto b = Q1_15::Approximate(-.5F);

  ASSERT_EQ((a + b).raw(), -Q1_15::Scale / 4);
}

TEST(FP_Q, AdditionOfDifferentTypes) {
  const auto a16 = Q1_15::Approximate(0.25F);
  const auto a32 = Q1_31::Approximate(0.25F);
  const auto b32 = Q1_31::Approximate(-0.125F);

  ASSERT_EQ((a16 + b32).raw(), Q1_31::Scale / 8);
  ASSERT_EQ((a16 + a32).raw(), Q1_31::Scale / 2);
}

TEST(FP_Q, AdditionOfInteger) {
  const auto a = Q8_8::Approximate(1.5F);

  ASSERT_EQ((a + 2).raw(), Q8_8::Scale * 3.5F);
  ASSERT_EQ((a + (-2)).raw(), -Q8_8::Scale / 2);
}

TEST(FP_Q, MultiplicationOfSameType) {
  const auto a = Q1_31::Approximate(0.5F);
  const auto b = Q1_31::Approximate(-0.25F);

  ASSERT_EQ((a * a).raw(), Q1_31::Scale / 4);
  ASSERT_EQ((a * b).raw(), -Q1_31::Scale / 8);
}

TEST(FP_Q, Reciprocal) {
  const auto a = Q8_8::Approximate(0.25F);
  const auto b = Q8_8::Approximate(-4.F);

  ASSERT_EQ(a.Reciprocal().raw(), 4 * Q8_8::Scale);
  ASSERT_EQ(b.Reciprocal().raw(), -Q8_8::Scale / 4);
}

TEST(FP_Q, Negate) {
  const auto a = Q8_8::Approximate(4.F);
  const auto b = Q8_8::Approximate(-2.F);

  ASSERT_EQ((-a).raw(), -4 * Q8_8::Scale);
  ASSERT_EQ((-b).raw(), 2 * Q8_8::Scale);
}