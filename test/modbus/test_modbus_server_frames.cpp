#include <memory>

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