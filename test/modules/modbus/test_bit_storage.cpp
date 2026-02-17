#include <gtest/gtest.h>

import modbus.server;

using namespace modbus::server;

TEST(BitStorage, ReadWriteInMemCoil) {
  uint8_t coil{};

  EXPECT_TRUE(WriteBit(coil, true).has_value());
  EXPECT_TRUE(*ReadBit(coil));

  EXPECT_TRUE(WriteBit(coil, false).has_value());
  EXPECT_FALSE(*ReadBit(coil));
}
