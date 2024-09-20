#include <gtest/gtest.h>

#include <constexpr_tools/math.h>


TEST(ConstexprMath, NumDigitsBase10) {
  using namespace ct;

  // 1 digit
  ASSERT_EQ(NumDigits(0), 1);

  // 2 digits
  ASSERT_EQ(NumDigits(10), 2);
  ASSERT_EQ(NumDigits(11), 2);
  ASSERT_EQ(NumDigits(45), 2);
  ASSERT_EQ(NumDigits(99), 2);

  // 3 digits
  ASSERT_EQ(NumDigits(100), 3);
  ASSERT_EQ(NumDigits(361), 3);
  ASSERT_EQ(NumDigits(999), 3);

  // Many digits
  ASSERT_EQ(NumDigits(123'456), 6);
  ASSERT_EQ(NumDigits(999'999), 6);
  ASSERT_EQ(NumDigits(10'000'000), 8);

  // Max value
  ASSERT_EQ(NumDigits<uint16_t>(std::numeric_limits<uint16_t>::max(), 10), 5);
}

TEST(ConstexprMath, Pow2) {
  using namespace ct;

  // Edge cases
  ASSERT_FLOAT_EQ(Pow2(0), 1.F);
  ASSERT_FLOAT_EQ(Pow2(1), 2.F);

  // Positive exponent
  ASSERT_FLOAT_EQ(Pow2(2), 4.F);
  ASSERT_FLOAT_EQ(Pow2(4), 16.F);
  ASSERT_FLOAT_EQ(Pow2(9), 512.F);
  ASSERT_FLOAT_EQ(Pow2(31), 2147483648.F);

  // Negative exponent
  ASSERT_FLOAT_EQ(Pow2(-1), 0.5F);
  ASSERT_FLOAT_EQ(Pow2(-3), 0.125F);
  ASSERT_FLOAT_EQ(Pow2(-16), 1.52587890625e-05F);
}

