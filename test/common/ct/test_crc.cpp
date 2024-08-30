#include <gtest/gtest.h>

#include <constexpr_tools/crc.h>


TEST(Crc, Crc16) {
  std::array<std::byte, 4> data{std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD}};
  const auto crc = ct::Crc16(data);

  ASSERT_EQ(crc, 0);
}
