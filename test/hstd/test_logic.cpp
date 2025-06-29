#include <gtest/gtest.h>

import hstd;

TEST(HstdLogic, Implies) {
  ASSERT_TRUE(hstd::Implies(false, false));
  ASSERT_TRUE(hstd::Implies(false, true));
  ASSERT_FALSE(hstd::Implies(true, false));
  ASSERT_TRUE(hstd::Implies(true, true));
}

TEST(HstdLogic, Xor) {
  ASSERT_FALSE(hstd::Xor(false, false));
  ASSERT_TRUE(hstd::Xor(false, true));
  ASSERT_TRUE(hstd::Xor(true, false));
  ASSERT_FALSE(hstd::Xor(true, true));
}
