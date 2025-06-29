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

using DiscreteInput0  = InMemDiscreteInput<0x0000, "DiscreteInput1">;
using DiscreteInput1  = InMemDiscreteInput<0x0001, "DiscreteInput2">;
using DiscreteInput4  = InMemDiscreteInput<0x0004, "DiscreteInput4">;
using DiscreteInput7  = InMemDiscreteInput<0x0007, "DiscreteInput7">;
using DiscreteInputs1 = InMemDiscreteInputSet<0x0008, 8, "DiscreteInputs1">;

using Coil1      = InMemCoil<0x0000, "Coil1">;
using Coil2      = InMemCoil<0x0001, "Coil2">;
using Coil4      = InMemCoil<0x0004, "Coil4">;
using Coil7      = InMemCoil<0x0007, "Coil7">;
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

using Srv =
    Server<hstd::Types<DiscreteInput0, DiscreteInput1, DiscreteInput4,
                       DiscreteInput7, DiscreteInputs1>,
           hstd::Types<Coil2, Coil1, Coil4, Coil7, CoilGroup1, CoilGroup2>,
           hstd::Types<>,
           hstd::Types<U16HR1, U16HR2, U16ArrayHR, F32HR1, F32HR2, F32ArrayHR>>;

using namespace hstd::literals;

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

TEST_F(ModbusServerFrames, ReadDiscreteInputsSingleByte) {
  server().GetStorage<DiscreteInput1>() = 1;
  server().GetStorage<DiscreteInput4>() = 1;
  server().GetStorage<DiscreteInput7>() = 1;

  const auto response = HandleFrame(ReadDiscreteInputsRequest{
      .starting_addr = 0,
      .num_inputs    = 8,
  });

  using Pdu = ReadDiscreteInputsResponse;
  ASSERT_THAT(response,
              VariantWith<Pdu>(
                  Field(&Pdu::inputs, ElementsAre(std::byte{0b1001'0010}))));
}

TEST_F(ModbusServerFrames, ReadDiscreteInputsMultipleBytes) {
  server().GetStorage<DiscreteInput1>()  = 1;
  server().GetStorage<DiscreteInput4>()  = 1;
  server().GetStorage<DiscreteInput7>()  = 1;
  server().GetStorage<DiscreteInputs1>() = 0b1001;

  const auto response = HandleFrame(ReadDiscreteInputsRequest{
      .starting_addr = 0,
      .num_inputs    = 16,
  });

  using Pdu = ReadDiscreteInputsResponse;
  ASSERT_THAT(response,
              VariantWith<Pdu>(Field(
                  &Pdu::inputs, ElementsAre(0b1001'0010_b, 0b0000'1001_b))));
}

TEST_F(ModbusServerFrames, ReadDiscreteInputsInvalidAddress) {
  const auto response = HandleFrame(ReadDiscreteInputsRequest{
      .starting_addr = 0xEEEE,
      .num_inputs    = 8,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x82),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::IllegalDataAddress))));
}

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

TEST_F(ModbusServerFrames, ReadCoilsMultipleBytes) {
  server().WriteCoil(0, true);
  server().WriteCoil(4, true);
  server().WriteCoil(7, true);
  server().WriteCoil(8, true);
  server().WriteCoil(11, true);

  const auto response = HandleFrame(ReadCoilsRequest{
      .starting_addr = 0,
      .num_coils     = 16,
  });

  ASSERT_THAT(response, VariantWith<ReadCoilsResponse>(
                            Field(&ReadCoilsResponse::coils,
                                  ElementsAre(0b1001'0001_b, 0b0000'1001_b))));
}

TEST_F(ModbusServerFrames, ReadCoilsInvalidAddress) {
  const auto response = HandleFrame(ReadCoilsRequest{
      .starting_addr = 0xEEEE,
      .num_coils     = 8,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x81),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::IllegalDataAddress))));
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
  std::array<std::byte, 2> read_coils{};
  ASSERT_THAT(server().ReadCoils(0x20, 16, read_coils),
              Optional(ElementsAreArray(coils)));
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
  server().WriteHoldingRegister(0x0001, 0x1234_u16);

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
  server().WriteHoldingRegister(0x0004, 0x1234_u16);
  server().WriteHoldingRegister(0x0005, 0x5678_u16);

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
  server().WriteHoldingRegister(0x0010, v);

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
    server().WriteHoldingRegister(0x0018 + i * 2, arr[i]);
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

TEST_F(ModbusServerFrames, WriteSingleRegisterOk) {
  const auto response = HandleFrame(
      WriteSingleRegisterRequest{.register_addr = 0x0001, .new_value = 0x1234});

  // Validate response
  using Pdu = WriteSingleRegisterResponse;
  ASSERT_THAT(response,
              VariantWith<Pdu>(AllOf(Field(&Pdu::register_addr, 0x0001),
                                     Field(&Pdu::new_value, 0x1234))));

  // Validate that value was written
  const auto value = server().ReadHoldingRegister<uint16_t>(0x0001);
  ASSERT_THAT(value, Optional(0x1234_u16));
}

TEST_F(ModbusServerFrames, WriteSingleRegisterInvalidAddress) {
  const auto response = HandleFrame(
      WriteSingleRegisterRequest{.register_addr = 0xEEEE, .new_value = 0x1234});

  // Validate that value was written
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x86),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::IllegalDataAddress))));
}

TEST_F(ModbusServerFrames, WriteMultipleRegistersSingleRegister) {
  std::array<std::byte, 2> data{0xAB_b, 0xCD_b};
  const auto               response = HandleFrame(WriteMultipleRegistersRequest{
                    .start_addr = 0x0001, .num_registers = 1, .values = data});

  // Validate the response
  using Pdu = WriteMultipleRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(AllOf(Field(&Pdu::start_addr, 0x0001),
                                               Field(&Pdu::num_registers, 1))));

  // Validate that the value was written
  ASSERT_THAT(server().ReadHoldingRegister<uint16_t>(0x0001), Optional(0xABCD));
}

TEST_F(ModbusServerFrames, WriteMultipleRegistersFloatRegister) {
  constexpr float Val  = 12.34F;
  constexpr auto  Data = hstd::ToByteArray<std::endian::big>(Val);

  const auto response = HandleFrame(WriteMultipleRegistersRequest{
      .start_addr    = 0x0010,
      .num_registers = 2,
      .values        = Data,
  });

  // Validate the response
  using Pdu = WriteMultipleRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(AllOf(Field(&Pdu::start_addr, 0x0010),
                                               Field(&Pdu::num_registers, 2))));

  // Validate that the value was written
  ASSERT_THAT(server().ReadHoldingRegister<float>(0x0010), Optional(Val));
}

TEST_F(ModbusServerFrames, WriteMultipleRegistersFloatArray) {
  constexpr std::array<float, 4> Vals{1.2F, 3.4F, 5.6F, 7.8F};
  constexpr auto Data = hstd::ToByteArray<std::endian::big>(Vals);

  const auto response = HandleFrame(WriteMultipleRegistersRequest{
      .start_addr    = 0x0018,
      .num_registers = 8,
      .values        = Data,
  });

  // Validate the response
  using Pdu = WriteMultipleRegistersResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(AllOf(Field(&Pdu::start_addr, 0x0018),
                                               Field(&Pdu::num_registers, 8))));

  // Validate that the value was written
  ASSERT_THAT((server().ReadHoldingRegister<std::array<float, 4>>(0x0018)),
              Optional(ElementsAreArray(Vals)));
}

TEST_F(ModbusServerFrames, WriteMultipleRegistersDataSizeDoesntMatchRegCount) {
  constexpr float Val  = 12.34F;
  constexpr auto  Data = hstd::ToByteArray<std::endian::big>(Val);

  const auto response = HandleFrame(WriteMultipleRegistersRequest{
      .start_addr    = 0x0010,
      .num_registers = 5,
      .values        = Data,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response,
              VariantWith<Pdu>(AllOf(Field(&Pdu::function_code, 0x90),
                                     Field(&Pdu::exception_code,
                                           ExceptionCode::IllegalDataValue))));
}

TEST_F(ModbusServerFrames, WriteMultipleRegistersUnalignedWrite) {
  constexpr float Val  = 12.34F;
  constexpr auto  Data = hstd::ToByteArray<std::endian::big>(Val);

  const auto response = HandleFrame(WriteMultipleRegistersRequest{
      .start_addr    = 0x0011,
      .num_registers = 2,
      .values        = Data,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x90),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::ServerDeviceFailure))));
}

TEST_F(ModbusServerFrames, WriteMultipleRegistersInvalidAddress) {
  constexpr float Val  = 12.34F;
  constexpr auto  Data = hstd::ToByteArray<std::endian::big>(Val);

  const auto response = HandleFrame(WriteMultipleRegistersRequest{
      .start_addr    = 0xEEEE,
      .num_registers = 2,
      .values        = Data,
  });

  // Validate response
  using Pdu = ErrorResponse;
  ASSERT_THAT(response, VariantWith<Pdu>(
                            AllOf(Field(&Pdu::function_code, 0x90),
                                  Field(&Pdu::exception_code,
                                        ExceptionCode::IllegalDataAddress))));
}
