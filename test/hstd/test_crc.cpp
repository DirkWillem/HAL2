#include <gtest/gtest.h>

import hstd;

TEST(HstdCrc, Crc16) {
  std::array<std::byte, 4> data{std::byte{0xAA}, std::byte{0xBB},
                                std::byte{0xCC}, std::byte{0xDD}};
  const auto               crc = hstd::Crc16(data);

  ASSERT_EQ(crc, 0xA4C4);
}
