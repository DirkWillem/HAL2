#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import hstd;

import modbus.core;
import modbus.server;

import testing.helpers;

using namespace testing;

using namespace modbus;
using namespace modbus::server;

using Coil1      = modbus::server::InMemCoil<0x0000, "Coil1">;
using Coil2      = modbus::server::InMemCoil<0x0001, "Coil2">;
using Coil4      = modbus::server::InMemCoil<0x0004, "Coil4">;
using Coil7      = modbus::server::InMemCoil<0x0007, "Coil7">;
using CoilGroup1 = modbus::server::InMemCoilSet<0x0008, 4, "Coils">;
using CoilGroup2 = modbus::server::InMemCoilSet<0x0020, 16, "Coils2">;

using U16HR1 = InMemHoldingRegister<0x0000, uint16_t, "U16 HR 1">;
using U16HR2 = InMemHoldingRegister<0x0001, uint16_t, "U16 HR 2">;

using U16ArrayHR =
    InMemHoldingRegister<0x0004, std::array<uint16_t, 4>, "U16 Array HR">;

using F32HR1 = InMemHoldingRegister<0x0010, float, "F32 HR 1">;
using F32HR2 = InMemHoldingRegister<0x0012, float, "F32 HR 2">;

using F32ArrayHR =
    InMemHoldingRegister<0x0018, std::array<float, 4>, "F32 Array HR">;

using Srv = modbus::server::Server<
    hstd::Types<Coil2, Coil1, Coil4, Coil7, CoilGroup1, CoilGroup2>,
    hstd::Types<U16HR1, U16HR2, U16ArrayHR, F32HR1, F32HR2, F32ArrayHR>>;

using namespace hstd::operators;

class ModbusServerFrames : public Test {
 public:
  void SetUp() override { srv = std::make_unique<Srv>(); }

  ResponsePdu<FrameVariant::Encode>
  HandleFrame(RequestPdu<FrameVariant::Decode> request) noexcept {
    ResponsePdu<FrameVariant::Encode> response{};
    srv->HandleFrame(request, response);
    return response;
  }

  Srv& server() & noexcept { return *srv; }

 private:
  std::unique_ptr<Srv> srv{nullptr};
};

TEST_F(ModbusServerFrames, ReadCoilsFrameSingleByte) {
  server().WriteCoil(0, true);
  server().WriteCoil(4, true);
  server().WriteCoil(7, true);

  const auto response = HandleFrame(ReadCoilsRequest{
      .starting_addr = 0,
      .num_coils     = 8,
  });

  ASSERT_THAT(response, VariantWith<ReadCoilsResponse>(
                            Field(&ReadCoilsResponse::coils,
                                  ElementsAre(std::byte{0b1001'0001}))));
}

TEST_F(ModbusServerFrames, WriteSingleCoilToOn) {
  // Handle frame
  const auto response = HandleFrame(WriteSingleCoilRequest{
      .coil_addr = 1,
      .new_state = CoilState::Enabled,
  });

  // Validate response
  using Pdu = WriteSingleCoilResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::coil_addr, 1),
                                  Field(&Pdu::new_state, CoilState::Enabled))));

  // Validate written coil state
  const auto coil_value = server().ReadCoil(1);
  ASSERT_THAT(coil_value, Optional(true));
}

TEST_F(ModbusServerFrames, WriteSingleCoilToOff) {
  // Set coil to ON
  server().WriteCoil(4, true);

  // Handle frame
  const auto response = HandleFrame(WriteSingleCoilRequest{
      .coil_addr = 4,
      .new_state = CoilState::Disabled,
  });

  // Validate response
  using Pdu = WriteSingleCoilResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(AllOf(
                            Field(&Pdu::coil_addr, 4),
                            Field(&Pdu::new_state, CoilState::Disabled))));

  // Validate coil state
  const auto coil_value = server().ReadCoil(4);
  ASSERT_THAT(coil_value, Optional(false));
}

TEST_F(ModbusServerFrames, WriteSingleCoilInvalidState) {
  // Handle frame
  const auto response = HandleFrame(WriteSingleCoilRequest{
      .coil_addr = 4,
      .new_state = static_cast<CoilState>(0xAAAA),
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response,
              VariantWith<Pdu>(AllOf(Field(&Pdu::function_code, 0x85),
                                     Field(&Pdu::exception_code,
                                           ExceptionCode::IllegalDataValue))));
}

TEST_F(ModbusServerFrames, WriteSingleCoilInvalidAddress) {
  // Handle frame
  const auto response = HandleFrame(WriteSingleCoilRequest{
      .coil_addr = 0xAAAA,
      .new_state = CoilState::Enabled,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x85),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::IllegalDataAddress))));
}

TEST_F(ModbusServerFrames, WriteMultipleCoils) {
  std::array<std::byte, 2> coils{std::byte{0b11110000}, std::byte{0b10100101}};

  // Handle frame
  const auto response = HandleFrame(WriteMultipleCoilsRequest{
      .start_addr = 0x0020,
      .num_coils  = 16,
      .values     = coils,
  });

  // Validate response
  using Pdu = WriteMultipleCoilsResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(AllOf(Field(&Pdu::start_addr, 0x0020),
                                               Field(&Pdu::num_coils, 16))));

  // Validate that coils are written
  ASSERT_THAT(server().ReadCoils<uint16_t>(0x20, 16),
              Optional(0b10100101'11110000));
}

TEST_F(ModbusServerFrames, WriteMultipleCoilsToInvalidAddress) {
  std::array<std::byte, 2> coils{std::byte{0b11110000}, std::byte{0b10100101}};

  // Handle frame
  const auto response = HandleFrame(WriteMultipleCoilsRequest{
      .start_addr = 0x2000,
      .num_coils  = 16,
      .values     = coils,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x8F),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::IllegalDataAddress))));
}

TEST_F(ModbusServerFrames,
       WriteMultipleCoilsMismatchBetweenValuesAndCoilCount) {
  std::array<std::byte, 2> coils{};

  // Handle frame
  const auto response = HandleFrame(WriteMultipleCoilsRequest{
      .start_addr = 0x0020,
      .num_coils  = 24,
      .values     = coils,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response,
              VariantWith<Pdu>(AllOf(Field(&Pdu::function_code, 0x8F),
                                     Field(&Pdu::exception_code,
                                           ExceptionCode::IllegalDataValue))));
}

TEST_F(ModbusServerFrames, ReadHoldingRegistersSingleRegister) {
  // Write sample value
  server().WriteHoldingRegister(0x1234_u16, 0x0001);

  // Handle frame
  const auto response = HandleFrame(ReadHoldingRegistersRequest{
      .starting_addr         = 0x0001,
      .num_holding_registers = 1,
  });

  // Validate response
  using Pdu = ReadHoldingRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(Field(&Pdu::registers,
                                               ElementsAre(0x12_b, 0x34_b))));
}

TEST_F(ModbusServerFrames, ReadHoldingRegisterMultipleRegisters) {
  server().WriteHoldingRegister(0x1234_u16, 0x0004);
  server().WriteHoldingRegister(0x5678_u16, 0x0005);

  // Handle frame
  const auto response = HandleFrame(ReadHoldingRegistersRequest{
      .starting_addr         = 0x0004,
      .num_holding_registers = 2,
  });

  // Validate response
  using Pdu = ReadHoldingRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(Field(
                            &Pdu::registers,
                            ElementsAre(0x12_b, 0x34_b, 0x56_b, 0x78_b))));
}

TEST_F(ModbusServerFrames, ReadHoldingRegisterReadFloat) {
  // Write sample value
  const auto v = 123.456F;
  server().WriteHoldingRegister(v, 0x0010);

  // Handle frame
  const auto response = HandleFrame(ReadHoldingRegistersRequest{
      .starting_addr         = 0x0010,
      .num_holding_registers = 2,
  });

  // Validate response
  std::array<std::byte, 4> v_check;
  helpers::BufferBuilder<std::endian::big>{v_check}.Write(v);

  using Pdu = ReadHoldingRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            Field(&Pdu::registers, ElementsAreArray(v_check))));
}

TEST_F(ModbusServerFrames, ReadHoldingRegisterReadFloats) {
  // Write sample value
  std::array<float, 4> arr{1.2F, 3.4F, 5.6F, 7.8F};
  for (std::size_t i = 0; i < arr.size(); ++i) {
    server().WriteHoldingRegister(arr[i], 0x0018 + i * 2);
  }

  // Handle frame
  const auto response = HandleFrame(ReadHoldingRegistersRequest{
      .starting_addr         = 0x0018,
      .num_holding_registers = 8,
  });

  // Validate response
  std::array<std::byte, arr.size() * sizeof(float)> v_check;
  helpers::BufferBuilder<std::endian::big>          bb{v_check};
  for (const auto v : arr) {
    bb.Write(v);
  }

  using Pdu = ReadHoldingRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            Field(&Pdu::registers, ElementsAreArray(v_check))));
}

TEST_F(ModbusServerFrames, ReadHoldingRegisterUnalignedRead) {
  // Handle frame
  const auto response = HandleFrame(ReadHoldingRegistersRequest{
      .starting_addr         = 0x0010,
      .num_holding_registers = 1,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x83),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::ServerDeviceFailure))));
}
