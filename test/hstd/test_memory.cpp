#include <bit>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import hstd;

using namespace testing;
using namespace hstd::literals;

TEST(HstdMemory, ToByteArrayScalar) {
  EXPECT_THAT(hstd::ToByteArray<std::endian::little>(0x12_u8),
              ElementsAre(0x12_b));
  EXPECT_THAT(hstd::ToByteArray<std::endian::little>(0x12345678_u32),
              ElementsAre(0x78_b, 0x56_b, 0x34_b, 0x12_b));

  EXPECT_THAT(hstd::ToByteArray<std::endian::big>(0x12_u8),
              ElementsAre(0x12_b));
  EXPECT_THAT(hstd::ToByteArray<std::endian::big>(0x12345678_u32),
              ElementsAre(0x12_b, 0x34_b, 0x56_b, 0x78_b));
}

TEST(HstdMemory, ToByteArrayArray) {
  EXPECT_THAT(hstd::ToByteArray<std::endian::little>(
                  std::array<uint16_t, 2>{0x1234, 0x5678}),
              ElementsAre(0x34_b, 0x12_b, 0x78_b, 0x56_b));
  EXPECT_THAT(hstd::ToByteArray<std::endian::big>(
                  std::array<uint16_t, 2>{0x1234, 0x5678}),
              ElementsAre(0x12_b, 0x34_b, 0x56_b, 0x78_b));
}