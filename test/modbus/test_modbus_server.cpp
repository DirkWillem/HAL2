#include <gmock/gmock.h>
#include <gtest/gtest.h>

import hstd;

import modbus.core;
import modbus.server;

using namespace testing;

using namespace modbus;
using namespace modbus::server;

using Coil1      = modbus::server::InMemCoil<0x0000, "Coil1">;
using Coil2      = modbus::server::InMemCoil<0x0001, "Coil2">;
using CoilGroup1 = modbus::server::InMemCoilSet<0x0004, 4, "Coils">;

using Srv = modbus::server::Server<hstd::Types<Coil2, Coil1, CoilGroup1>>;

TEST(ModbusServer, WriteCoilSingleCoilOk) {
  Srv srv{};

  EXPECT_THAT(srv.WriteCoil(0, true), Optional(true));
  EXPECT_THAT(srv.WriteCoil(1, false), Optional(false));
}

TEST(ModbusServer, WriteCoilInSetOk) {
  Srv srv{};

  EXPECT_THAT(srv.WriteCoil(5, true), Optional(true));
  EXPECT_THAT(srv.WriteCoil(6, false), Optional(false));
}

TEST(ModbusServer, WriteNonExistentCoil) {
  Srv srv{};

  const auto result = srv.WriteCoil(3, true);
  ASSERT_TRUE(!result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST(ModbusServer, ReadCoilSingleCoilOk) {
  Srv srv{};

  srv.WriteCoil(1, true);

  EXPECT_THAT(srv.ReadCoil(0), Optional(false));
  EXPECT_THAT(srv.ReadCoil(1), Optional(true));
}

TEST(ModbusServer, ReadCoilFromSetOk) {
  Srv srv{};

  srv.WriteCoil(7, true);

  EXPECT_THAT(srv.ReadCoil(6), Optional(false));
  EXPECT_THAT(srv.ReadCoil(7), Optional(true));
}

TEST(ModbusServer, ReadNonExistentCoil) {
  Srv srv{};

  const auto result = srv.ReadCoil(3);
  ASSERT_TRUE(!result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST(ModbusServer, ReadCoilsSingleCoilsSet) {
  Srv srv{};

  srv.WriteCoil(1, true);

  const auto result = srv.ReadCoils<uint32_t>(0, 4);
  ASSERT_THAT(result, Optional(0b0010));
}

TEST(ModbusServer, ReadCoilsCoilSetAligned) {
  Srv srv{};

  srv.WriteCoil(4, true);
  srv.WriteCoil(7, true);

  const auto result = srv.ReadCoils<uint32_t>(4, 4);
  ASSERT_THAT(result, Optional(0b1001));
}

TEST(ModbusServer, ReadCoilsCoilSetUnaligned) {
  Srv srv{};

  srv.WriteCoil(4, true);
  srv.WriteCoil(7, true);

  // Start reading before coil set
  const auto result = srv.ReadCoils<uint32_t>(2, 6);
  ASSERT_THAT(result, Optional(0b100100));

  // Start reading after coil set
  const auto result2 = srv.ReadCoils<uint32_t>(5, 4);
  ASSERT_THAT(result2, Optional(0b100));
}

TEST(ModbusServer, ReadCoilsMultipleEntries) {
  Srv srv{};

  srv.WriteCoil(0, true);
  srv.WriteCoil(4, true);
  srv.WriteCoil(7, true);

  const auto result = srv.ReadCoils<uint32_t>(0, 8);
  ASSERT_THAT(result, Optional(0b1001'0001));
}

TEST(ModbusServer, ReadCoilsNoEntries) {
  Srv srv{};

  const auto result = srv.ReadCoils<uint32_t>(8, 8);
  ASSERT_FALSE(result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

