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

