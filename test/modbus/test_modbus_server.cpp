#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import hstd;

import modbus.core;
import modbus.server;

using namespace testing;

using namespace modbus;
using namespace modbus::server;

using namespace hstd::literals;

using DiscreteInput1 = InMemDiscreteInput<0x0000, "DiscreteInput1">;
using DiscreteInput2 = InMemDiscreteInput<0x0001, "DiscreteInput2">;

using Coil1      = InMemCoil<0x0000, "Coil1">;
using Coil2      = InMemCoil<0x0001, "Coil2">;
using CoilGroup1 = InMemCoilSet<0x0008, 4, "Coils">;
using CoilGroup2 = InMemCoilSet<0x0020, 16, "Coils2">;

using U16HR1 = InMemHoldingRegister<0x0000, uint16_t, "U16 HR 1">;
using U16HR2 = InMemHoldingRegister<0x0001, uint16_t, "U16 HR 2">;

using U16ArrayHR =
    InMemHoldingRegister<0x0004, std::array<uint16_t, 4>, "U16 Array HR">;

using F32HR1 = InMemHoldingRegister<0x0010, float, "F32 HR 1">;
using F32HR2 = InMemHoldingRegister<0x0012, float, "F32 HR 2">;

using F32ArrayHR =
    InMemHoldingRegister<0x0018, std::array<float, 4>, "F32 Array HR">;

using U16IR0 = InMemInputRegister<0x0000, uint16_t, "U16 IR 1">;
using U16IR1 = InMemInputRegister<0x0001, uint16_t, "U16 IR 2">;

using U16ArrayIR =
    InMemInputRegister<0x0004, std::array<uint16_t, 4>, "U16 Array IR">;

using F32IR0 = InMemInputRegister<0x0010, float, "F32 IR 1">;
using F32IR1 = InMemInputRegister<0x0012, float, "F32 IR 2">;

using F32ArrayIR =
    InMemInputRegister<0x0018, std::array<float, 4>, "F32 Array IR">;

using Srv =
    Server<hstd::Types<DiscreteInput1, DiscreteInput2>,
           hstd::Types<Coil2, Coil1, CoilGroup1, CoilGroup2>,
           hstd::Types<U16IR0, U16IR1, U16ArrayIR, F32IR0, F32IR1, F32ArrayIR>,
           hstd::Types<U16HR1, U16HR2, U16ArrayHR, F32HR1, F32HR2, F32ArrayHR>>;

class ModbusServer : public Test {
 public:
  void SetUp() override { srv = std::make_unique<Srv>(); }

  std::unique_ptr<Srv> srv{nullptr};
};

TEST_F(ModbusServer, ReadSingleDiscreteInput) {
  srv->GetStorage<DiscreteInput2>() = 1;

  ASSERT_THAT(srv->ReadDiscreteInput(0), Optional(false));
  ASSERT_THAT(srv->ReadDiscreteInput(1), Optional(true));
}

TEST_F(ModbusServer, ReadNonExistentDiscreteInput) {
  const auto result = srv->ReadDiscreteInput(3);
  ASSERT_TRUE(!result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST_F(ModbusServer, ReadDiscreteInputs) {
  srv->GetStorage<DiscreteInput2>() = 1;

  std::array<std::byte, 1> into{};

  const auto result = srv->ReadDiscreteInputs(0, 8, into);
  ASSERT_THAT(result, Optional(ElementsAre(0b0000'0010_b)));
}

TEST_F(ModbusServer, WriteCoilSingleCoilOk) {
  EXPECT_THAT(srv->WriteCoil(0, true), Optional(true));
  EXPECT_THAT(srv->WriteCoil(1, false), Optional(false));
}

TEST_F(ModbusServer, WriteCoilInSetOk) {
  EXPECT_THAT(srv->WriteCoil(8, true), Optional(true));
  EXPECT_THAT(srv->WriteCoil(9, false), Optional(false));
}

TEST_F(ModbusServer, WriteNonExistentCoil) {
  const auto result = srv->WriteCoil(3, true);
  ASSERT_TRUE(!result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST_F(ModbusServer, WriteCoilsSingleCoilsOk) {
  std::array<std::byte, 1> data{std::byte{0b0000'0011}};

  const auto result = srv->WriteCoils(0, 8, data);

  ASSERT_THAT(result, Optional(true));

  ASSERT_THAT(srv->ReadCoil(0), Optional(true));
  ASSERT_THAT(srv->ReadCoil(1), Optional(true));
}

TEST_F(ModbusServer, WriteCoilsCoilSet) {
  std::array<std::byte, 2> data{std::byte{0b11110000}, std::byte{0b10100101}};

  const auto result = srv->WriteCoils(0x20, 16, data);

  ASSERT_THAT(result, Optional(true));

  std::array<std::byte, 2> data_read{};

  ASSERT_THAT(srv->ReadCoils(0x20, 16, data_read),
              Optional(ElementsAreArray(data)));
}

TEST_F(ModbusServer, ReadCoilSingleCoilOk) {
  srv->WriteCoil(1, true);

  EXPECT_THAT(srv->ReadCoil(0), Optional(false));
  EXPECT_THAT(srv->ReadCoil(1), Optional(true));
}

TEST_F(ModbusServer, ReadCoilFromSetOk) {
  srv->WriteCoil(9, true);

  EXPECT_THAT(srv->ReadCoil(8), Optional(false));
  EXPECT_THAT(srv->ReadCoil(9), Optional(true));
}

TEST_F(ModbusServer, ReadNonExistentCoil) {
  const auto result = srv->ReadCoil(3);
  ASSERT_TRUE(!result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST_F(ModbusServer, ReadCoilsNoEntries) {
  std::array<std::byte, 1> into{};
  const auto               result = srv->ReadCoils(100, 8, into);
  ASSERT_FALSE(result.has_value());
  ASSERT_EQ(result.error(), ExceptionCode::IllegalDataAddress);
}

TEST_F(ModbusServer, ReadCoils) {
  srv->WriteCoil(0, true);
  srv->WriteCoil(1, true);

  std::array<std::byte, 1> into{};

  const auto result = srv->ReadCoils(0, 8, into);
  ASSERT_THAT(result, Optional(ElementsAre(0b0000'0011_b)));
}

TEST_F(ModbusServer, ReadCoilsUnaligned) {
  srv->WriteCoil(0, true);
  srv->WriteCoil(1, true);

  std::array<std::byte, 1> into{};

  const auto result = srv->ReadCoils(1, 7, into);
  ASSERT_THAT(result, Optional(ElementsAre(0b0000'0001_b)));
}

TEST_F(ModbusServer, ReadCoilsMultipleBytes) {
  srv->WriteCoil(0, true);
  srv->WriteCoil(1, true);
  srv->WriteCoil(8, true);
  srv->WriteCoil(11, true);

  std::array<std::byte, 2> into{};

  const auto result = srv->ReadCoils(0, 16, into);
  ASSERT_THAT(result, Optional(ElementsAre(0b0000'0011_b, 0b0000'1001_b)));
}

TEST_F(ModbusServer, ReadCoilsMultipleBytesUnaligned) {
  srv->WriteCoil(0, true);
  srv->WriteCoil(1, true);
  srv->WriteCoil(8, true);
  srv->WriteCoil(11, true);

  std::array<std::byte, 2> into{};

  const auto result = srv->ReadCoils(2, 16, into);
  ASSERT_THAT(result, Optional(ElementsAre(0b0100'0000_b, 0b0000'0010_b)));
}

TEST_F(ModbusServer, ReadInputRegisterSingleU16) {
  srv->GetStorage<U16IR1>() = 0x1234;

  uint16_t result{0};

  const auto read_result =
      srv->ReadInputRegisters(hstd::MutByteViewOver(result), 1, 1);

  ASSERT_TRUE(read_result.has_value());
  ASSERT_EQ(result, 0x1234);
}

TEST_F(ModbusServer, ReadInputRegisterSingleF32) {
  srv->GetStorage<F32IR0>() = 123.456F;

  float result{0.F};

  const auto read_result =
      srv->ReadInputRegisters(hstd::MutByteViewOver(result), 0x0010, 2);

  ASSERT_TRUE(read_result.has_value());
  ASSERT_EQ(result, 123.456F);
}

TEST_F(ModbusServer, WriteHoldingRegisterSingleU16) {
  const uint16_t value{0x1234};

  const auto result =
      srv->WriteHoldingRegisters(hstd::ByteViewOver(value), 0x0000, 1);

  ASSERT_THAT(result, Optional(true));
}

TEST_F(ModbusServer, ReadHoldingRegisterSingleU16) {
  const uint16_t value{0x1234};
  uint16_t       result{0};

  ASSERT_THAT(srv->WriteHoldingRegisters(hstd::ByteViewOver(value), 0x0001, 1),
              Optional(true));

  const auto read_result =
      srv->ReadHoldingRegisters(hstd::MutByteViewOver(result), 1, 1);

  ASSERT_TRUE(read_result.has_value());
  ASSERT_EQ(result, value);
}

TEST_F(ModbusServer, ReadWriteHoldingRegisterFloat) {
  const auto value = 1.23F;
  float      value_read{};

  const auto write_result =
      srv->WriteHoldingRegisters(hstd::ByteViewOver(value), 0x0010, 2);
  ASSERT_THAT(write_result, Optional(true));

  const auto read_result =
      srv->ReadHoldingRegisters(hstd::MutByteViewOver(value_read), 0x0010, 2);
  ASSERT_TRUE(read_result.has_value());

  ASSERT_EQ(value_read, value);
}

TEST_F(ModbusServer, ReadWriteHoldingRegistersInArray) {
  std::array<uint16_t, 3> values{2, 3};

  const auto write_result =
      srv->WriteHoldingRegisters(hstd::ByteViewOver(values), 0x0005, 2);
  ASSERT_THAT(write_result, Optional(true));

  std::array<uint16_t, 4> dst{};
  const auto              read_result =
      srv->ReadHoldingRegisters(hstd::MutByteViewOver(dst), 0x0004, 4);
  ASSERT_TRUE(read_result.has_value());
  ASSERT_THAT(dst, ElementsAre(0, 2, 3, 0));
}

TEST_F(ModbusServer, UnalignedReadSingleElement) {
  uint16_t dst{};

  const auto read_result =
      srv->ReadHoldingRegisters(hstd::MutByteViewOver(dst), 0x0011, 1);
  ASSERT_FALSE(read_result.has_value());
  ASSERT_EQ(read_result.error(), ExceptionCode::ServerDeviceFailure);
}

TEST_F(ModbusServer, UnalignedWriteSingleElement) {
  uint16_t dst{};

  const auto write_result =
      srv->WriteHoldingRegisters(hstd::ByteViewOver(dst), 0x0011, 1);
  ASSERT_FALSE(write_result.has_value());
  ASSERT_EQ(write_result.error(), ExceptionCode::ServerDeviceFailure);
}

TEST_F(ModbusServer, UnalignedReadInArray) {
  uint16_t dst{};

  const auto read_result =
      srv->ReadHoldingRegisters(hstd::MutByteViewOver(dst), 0x0019, 1);
  ASSERT_FALSE(read_result.has_value());
  ASSERT_EQ(read_result.error(), ExceptionCode::ServerDeviceFailure);
}

TEST_F(ModbusServer, UnalignedWriteInArray) {
  uint16_t dst{};

  const auto write_result =
      srv->WriteHoldingRegisters(hstd::ByteViewOver(dst), 0x0019, 1);
  ASSERT_FALSE(write_result.has_value());
  ASSERT_EQ(write_result.error(), ExceptionCode::ServerDeviceFailure);
}
