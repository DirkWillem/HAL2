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
using CoilGroup1 = modbus::server::InMemCoilSet<0x0008, 4, "Coils">;
using CoilGroup2 = modbus::server::InMemCoilSet<0x0020, 16, "Coils2">;

using Srv =
    modbus::server::Server<hstd::Types<Coil2, Coil1, CoilGroup1, CoilGroup2>>;

TEST(ModbusServer, WriteCoilSingleCoilOk) {
  Srv srv{};

  EXPECT_THAT(srv.WriteCoil(0, true), Optional(true));
  EXPECT_THAT(srv.WriteCoil(1, false), Optional(false));
}

TEST(ModbusServer, WriteCoilInSetOk) {
  Srv srv{};

  EXPECT_THAT(srv.WriteCoil(8, true), Optional(true));
  EXPECT_THAT(srv.WriteCoil(9, false), Optional(false));
}

TEST(ModbusServer, WriteNonExistentCoil) {
  Srv srv{};

  const auto result = srv.WriteCoil(3, true);
  ASSERT_TRUE(!result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST(ModbusServer, WriteCoilsSingleCoilsOk) {
  std::array<std::byte, 1> data{std::byte{0b0000'0011}};
  Srv                      srv{};

  const auto result = srv.WriteCoils(0, 8, data);

  ASSERT_THAT(result, Optional(true));

  ASSERT_THAT(srv.ReadCoil(0), Optional(true));
  ASSERT_THAT(srv.ReadCoil(1), Optional(true));
}

TEST(ModbusServer, WriteCoilsCoilSet) {
  std::array<std::byte, 2> data{std::byte{0b11110000}, std::byte{0b10100101}};
  Srv                      srv{};

  const auto result = srv.WriteCoils(0x20, 16, data);

  ASSERT_THAT(result, Optional(true));

  ASSERT_THAT(srv.ReadCoils<uint16_t>(0x20, 16), Optional(0b10100101'11110000));


}

TEST(ModbusServer, ReadCoilSingleCoilOk) {
  Srv srv{};

  srv.WriteCoil(1, true);

  EXPECT_THAT(srv.ReadCoil(0), Optional(false));
  EXPECT_THAT(srv.ReadCoil(1), Optional(true));
}

TEST(ModbusServer, ReadCoilFromSetOk) {
  Srv srv{};

  srv.WriteCoil(9, true);

  EXPECT_THAT(srv.ReadCoil(8), Optional(false));
  EXPECT_THAT(srv.ReadCoil(9), Optional(true));
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

  srv.WriteCoil(8, true);
  srv.WriteCoil(11, true);

  const auto result = srv.ReadCoils<uint32_t>(8, 4);
  ASSERT_THAT(result, Optional(0b1001));
}

TEST(ModbusServer, ReadCoilsCoilSetUnaligned) {
  Srv srv{};

  srv.WriteCoil(8, true);
  srv.WriteCoil(11, true);

  // Start reading before coil set
  const auto result = srv.ReadCoils<uint32_t>(6, 6);
  ASSERT_THAT(result, Optional(0b100100));

  // Start reading after coil set
  const auto result2 = srv.ReadCoils<uint32_t>(9, 4);
  ASSERT_THAT(result2, Optional(0b100));
}

TEST(ModbusServer, ReadCoilsMultipleEntries) {
  Srv srv{};

  srv.WriteCoil(0, true);
  srv.WriteCoil(8, true);
  srv.WriteCoil(11, true);

  const auto result = srv.ReadCoils<uint32_t>(0, 12);
  ASSERT_THAT(result, Optional(0b1001'0000'0001));
}

TEST(ModbusServer, ReadCoilsNoEntries) {
  Srv srv{};

  const auto result = srv.ReadCoils<uint32_t>(100, 8);
  ASSERT_FALSE(result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}
