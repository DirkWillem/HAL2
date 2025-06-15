#include <gtest/gtest.h>

import modbus.server;

using namespace modbus::server;

TEST(ModbusStorageCoilDiscreteInput, ReadWriteInMemCoil) {
  uint8_t coil{};

  EXPECT_TRUE(WriteCoil(coil, true).has_value());
  EXPECT_TRUE(*ReadCoil(coil));

  EXPECT_TRUE(WriteCoil(coil, false).has_value());
  EXPECT_FALSE(*ReadCoil(coil));
}
