#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import hstd;

import modbus.core;
import modbus.encoding.rtu;

import testing.helpers;

using namespace testing;
using namespace modbus;
using namespace modbus::encoding::rtu;

using namespace hstd::literals;

class ModbusRtuEncoder : public Test {
 public:
  void SetUp() override {
    std::fill(buffer.begin(), buffer.end(), std::byte{0});
    std::fill(check_buffer.begin(), buffer.end(), std::byte{0});
  }

  void TearDown() override { check_buffer_builder = nullptr; }

  std::span<const std::byte> EncodeRequest(RequestFrame frame) {
    auto encoder = Encoding::Encoder{frame.address, buffer};
    return std::visit(encoder, frame.pdu);
  }

  std::span<const std::byte> EncodeResponse(ResponseFrame frame) {
    auto encoder = Encoding::Encoder{frame.address, buffer};
    return std::visit(encoder, frame.pdu);
  }

  auto& CheckBuilder() & {
    if (check_buffer_builder != nullptr) {
      throw std::runtime_error{
          "Can only create one check buffer per test case"};
    }

    check_buffer_builder = std::make_unique<
        helpers::BufferBuilder<std::endian::big, std::endian::little>>(
        check_buffer,
        helpers::BufferBuilderSettings{.default_crc16_poly = 0xA001,
                                       .default_crc16_init = 0xFFFF});
    return *check_buffer_builder;
  }

 protected:
  std::array<std::byte, 256> buffer{};

  std::array<std::byte, 256> check_buffer{};
  std::unique_ptr<helpers::BufferBuilder<std::endian::big, std::endian::little>>
      check_buffer_builder{nullptr};
};

TEST_F(ModbusRtuEncoder, ErrorResponse) {
  const auto encoded_frame = EncodeResponse({
      .pdu     = MakeErrorResponse(FunctionCode::ReadCoils,
                                   ExceptionCode::IllegalDataValue),
      .address = 0xEE,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0xEE)
                                       .Write<uint8_t>(0x81)
                                       .Write<uint8_t>(0x03)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadCoilsRequest) {
  const auto encoded_frame = EncodeRequest({
      .pdu =
          ReadCoilsRequest{
              .starting_addr = 0x0013,
              .num_coils     = 0x0016,
          },
      .address = 0x05,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x05)
                                       .Write<uint8_t>(0x01)
                                       .Write<uint16_t>(0x0013)
                                       .Write<uint16_t>(0x0016)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadCoilsResponseSingleCoil) {
  std::array<std::byte, 1> coils{std::byte{0b0000'0001}};

  const auto encoded_frame = EncodeResponse({
      .pdu     = ReadCoilsResponse{.coils = coils},
      .address = 0x06,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x06)
                                       .Write<uint8_t>(0x01)
                                       .Write<uint8_t>(1)
                                       .Write<uint8_t>(0b0000'0001)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadCoilsResponseMultipleCoils) {
  std::array<std::byte, 2> coils{std::byte{0b1111'0000},
                                 std::byte{0b1010'0101}};

  const auto encoded_frame = EncodeResponse({
      .pdu     = ReadCoilsResponse{.coils = coils},
      .address = 0x06,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x06)
                                       .Write<uint8_t>(0x01)
                                       .Write<uint8_t>(2)
                                       .Write<uint8_t>(0b1111'0000)
                                       .Write<uint8_t>(0b1010'0101)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadDiscreteInputsRequest) {
  const auto encoded_frame = EncodeRequest({
      .pdu =
          ReadDiscreteInputsRequest{
              .starting_addr = 0x0014,
              .num_inputs    = 0x0017,
          },
      .address = 0x10,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x10)
                                       .Write<uint8_t>(0x02)
                                       .Write<uint16_t>(0x0014)
                                       .Write<uint16_t>(0x0017)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadDiscreteInputsResponse) {
  std::array inputs{std::byte{0b1111'0000}, std::byte{0b1010'0101}};

  const auto encoded_frame = EncodeResponse({
      .pdu     = ReadDiscreteInputsResponse{.inputs = inputs},
      .address = 0x06,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x06)
                                       .Write<uint8_t>(0x02)
                                       .Write<uint8_t>(2)
                                       .Write<uint8_t>(0b1111'0000)
                                       .Write<uint8_t>(0b1010'0101)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadHoldingRegistersRequest) {
  const auto encoded_frame = EncodeRequest({
      .pdu =
          ReadHoldingRegistersRequest{
              .starting_addr         = 0x0015,
              .num_holding_registers = 0x0018,
          },
      .address = 0x11,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x11)
                                       .Write<uint8_t>(0x03)
                                       .Write<uint16_t>(0x0015)
                                       .Write<uint16_t>(0x0018)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadHoldingRegistersResponse) {
  std::array registers{0x12_b, 0x34_b, 0xAA_b, 0xBB_b, 0xBE_b, 0xEF_b};

  const auto encoded_frame = EncodeResponse({
      .pdu     = ReadHoldingRegistersResponse{.registers = registers},
      .address = 0x11,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x11)
                                       .Write<uint8_t>(0x03)
                                       .Write<uint8_t>(0x06)
                                       .Write<uint16_t>(0x1234)
                                       .Write<uint16_t>(0xAABB)
                                       .Write<uint16_t>(0xBEEF)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadInputRegistersRequest) {
  const auto encoded_frame = EncodeRequest({
      .pdu =
          ReadInputRegistersRequest{
              .starting_addr       = 0x0016,
              .num_input_registers = 0x0019,
          },
      .address = 0x11,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x11)
                                       .Write<uint8_t>(0x04)
                                       .Write<uint16_t>(0x0016)
                                       .Write<uint16_t>(0x0019)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadInputRegistersResponse) {
  std::array<std::byte, 6> registers{0x12_b, 0x34_b, 0xAA_b,
                                     0xBB_b, 0xBE_b, 0xEF_b};

  const auto encoded_frame = EncodeResponse({
      .pdu =
          ReadInputRegistersResponse{
              .registers = registers,
          },
      .address = 0x11,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x11)
                                       .Write<uint8_t>(0x04)
                                       .Write<uint8_t>(0x06)
                                       .Write<uint16_t>(0x1234)
                                       .Write<uint16_t>(0xAABB)
                                       .Write<uint16_t>(0xBEEF)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteSingleRegisterRequest) {
  const auto encoded_frame = EncodeRequest({
      .pdu     = WriteSingleRegisterRequest{.register_addr = 0x0010,
                                            .new_value     = 0x1234},
      .address = 0x12,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x12)
                                       .Write<uint8_t>(0x06)
                                       .Write<uint16_t>(0x0010)
                                       .Write<uint16_t>(0x1234)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteSingleRegisterResponse) {
  const auto encoded_frame = EncodeResponse({
      .pdu     = WriteSingleRegisterResponse{.register_addr = 0x0010,
                                             .new_value     = 0x1234},
      .address = 0x12,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x12)
                                       .Write<uint8_t>(0x06)
                                       .Write<uint16_t>(0x0010)
                                       .Write<uint16_t>(0x1234)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteSingleCoilRequest) {
  const auto encoded_frame = EncodeRequest({
      .pdu     = WriteSingleCoilRequest{.coil_addr = 0x0020,
                                        .new_state = CoilState::Enabled},
      .address = 0x10,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x10)
                                       .Write<uint8_t>(0x05)
                                       .Write<uint16_t>(0x0020)
                                       .Write<uint16_t>(0xFF00)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteSingleCoilResponse) {
  const auto encoded_frame = EncodeResponse({
      .pdu     = WriteSingleCoilResponse{.coil_addr = 0x0030,
                                         .new_state = CoilState::Enabled},
      .address = 0x10,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x10)
                                       .Write<uint8_t>(0x05)
                                       .Write<uint16_t>(0x0030)
                                       .Write<uint16_t>(0xFF00)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteMultipleCoilsRequest) {
  std::array<std::byte, 2> coils{std::byte{0b1111'0000},
                                 std::byte{0b1010'0101}};

  const auto encoded_frame = EncodeRequest({
      .pdu     = WriteMultipleCoilsRequest{.start_addr = 0x0020,
                                           .num_coils  = 13,
                                           .values     = {coils}},
      .address = 0x06,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x06)
                                       .Write<uint8_t>(0x0F)
                                       .Write<uint16_t>(0x0020)
                                       .Write<uint16_t>(13)
                                       .Write<uint8_t>(2)
                                       .Write<uint8_t>(0b1111'0000)
                                       .Write<uint8_t>(0b1010'0101)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteMultipleCoilsResponse) {
  const auto encoded_frame = EncodeResponse(
      {.pdu = WriteMultipleCoilsResponse{.start_addr = 0x0020, .num_coils = 13},
       .address = 0x65});

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x65)
                                       .Write<uint8_t>(0x0F)
                                       .Write<uint16_t>(0x0020)
                                       .Write<uint16_t>(13)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteMultipleRegistersRequest) {
  std::array<std::byte, 4> values{0x12_b, 0x34_b, 0x56_b, 0x78_b};

  const auto encoded_frame = EncodeRequest({
      .pdu =
          WriteMultipleRegistersRequest{
              .start_addr    = 0x00A0,
              .num_registers = 2,
              .values        = values,
          },
      .address = 0x07,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x07)
                                       .Write<uint8_t>(0x10)
                                       .Write<uint16_t>(0x00A0)
                                       .Write<uint16_t>(2)
                                       .Write<uint8_t>(4)
                                       .Write<uint16_t>(0x1234)
                                       .Write<uint16_t>(0x5678)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, WriteMultipleRegistersResponse) {
  const auto encoded_frame = EncodeResponse({
      .pdu =
          WriteMultipleRegistersResponse{
              .start_addr    = 0x00A0,
              .num_registers = 2,
          },
      .address = 0x07,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x07)
                                       .Write<uint8_t>(0x10)
                                       .Write<uint16_t>(0x00A0)
                                       .Write<uint16_t>(2)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ElementsAreArray(encoded_frame_check));
}