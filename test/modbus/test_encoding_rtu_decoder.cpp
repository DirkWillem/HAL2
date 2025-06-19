#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import modbus.core;
import modbus.encoding.rtu;

import testing.helpers;

using namespace ::testing;

using namespace modbus;
using namespace modbus::encoding::rtu;

class ModbusRtuDecoder : public Test {
 public:
  void SetUp() override {
    std::fill(req_buffer.begin(), req_buffer.end(), std::byte{0});
  }

  void TearDown() override { req_buffer_builder = nullptr; }

  auto DecodeRequest(std::span<const std::byte> data) {
    auto decoder = modbus::encoding::rtu::Encoding::Decoder{data};
    return decoder.DecodeRequest();
  }

  auto DecodeResponse(std::span<const std::byte> data) {
    auto decoder = modbus::encoding::rtu::Encoding::Decoder{data};
    return decoder.DecodeResponse();
  }

  auto& FrameBuilder() & {
    if (req_buffer_builder != nullptr) {
      throw std::runtime_error{
          "Can only create one check buffer per test case"};
    }

    req_buffer_builder = std::make_unique<
        helpers::BufferBuilder<std::endian::big, std::endian::little>>(
        req_buffer,
        helpers::BufferBuilderSettings{.default_crc16_poly = 0xA001,
                                       .default_crc16_init = 0xFFFF});
    return *req_buffer_builder;
  }

 protected:
  std::array<std::byte, 256> req_buffer{};
  std::unique_ptr<helpers::BufferBuilder<std::endian::big, std::endian::little>>
      req_buffer_builder{nullptr};
};

TEST_F(ModbusRtuDecoder, TooLittleDataForAnyFrame) {
  const auto decode_result = DecodeRequest(
      FrameBuilder().Write<uint8_t>(0x05).Write<uint8_t>(0x01).Bytes());

  ASSERT_FALSE(decode_result.has_value());
  ASSERT_EQ(decode_result.error(),
            modbus::encoding::rtu::DecodeError::IncompleteCommand);
}

TEST_F(ModbusRtuDecoder, TooLittleDataForSpecificFrame) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x05)
                                               .Write<uint8_t>(0x01)
                                               .Write<uint16_t>(0x0013)
                                               .Write<uint8_t>(0)
                                               .Bytes());

  ASSERT_FALSE(decode_result.has_value());
  ASSERT_EQ(decode_result.error(),
            modbus::encoding::rtu::DecodeError::IncompleteCommand);
}

TEST_F(ModbusRtuDecoder, TooMuchData) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x05)
                                               .Write<uint8_t>(0x01)
                                               .Write<uint16_t>(0x0013)
                                               .Write<uint16_t>(0x0016)
                                               .Write<uint16_t>(0x0000)
                                               .Write<uint16_t>(0xAAAA)
                                               .Bytes());

  ASSERT_FALSE(decode_result.has_value());
  ASSERT_EQ(decode_result.error(),
            modbus::encoding::rtu::DecodeError::TooMuchData);
}

TEST_F(ModbusRtuDecoder, InvalidCRC) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x05)
                                               .Write<uint8_t>(0x01)
                                               .Write<uint16_t>(0x0013)
                                               .Write<uint16_t>(0x0016)
                                               .Write<uint16_t>(0xAAAA)
                                               .Bytes());

  ASSERT_FALSE(decode_result.has_value());
  ASSERT_EQ(decode_result.error(),
            modbus::encoding::rtu::DecodeError::InvalidCrc);
}

TEST_F(ModbusRtuDecoder, InvalidFunctionCode) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x01)
                                               .Write<uint8_t>(0x80)
                                               .Write<uint16_t>(0x0013)
                                               .Write<uint16_t>(0x0016)
                                               .Write<uint16_t>(0xAAAA)
                                               .Bytes());

  ASSERT_FALSE(decode_result.has_value());
  ASSERT_EQ(decode_result.error(),
            modbus::encoding::rtu::DecodeError::InvalidFunctionCode);
}

TEST_F(ModbusRtuDecoder, ErrorResponse) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0xEE)
                                                .Write<uint8_t>(0x81)
                                                .Write<uint8_t>(0x03)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0xEE);
  ASSERT_THAT(frame.pdu, VariantWith<modbus::ErrorResponse>(AllOf(
                             Field(&modbus::ErrorResponse::function_code, 0x81),
                             Field(&modbus::ErrorResponse::exception_code,
                                   modbus::ExceptionCode::IllegalDataValue))));
}

TEST_F(ModbusRtuDecoder, ReadCoilsRequest) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x05)
                                               .Write<uint8_t>(0x01)
                                               .Write<uint16_t>(0x0013)
                                               .Write<uint16_t>(0x0016)
                                               .WriteCrc16()
                                               .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x05);
  ASSERT_THAT(frame.pdu,
              VariantWith<modbus::ReadCoilsRequest>(
                  AllOf(Field(&modbus::ReadCoilsRequest::starting_addr, 0x0013),
                        Field(&modbus::ReadCoilsRequest::num_coils, 0x0016))));
}

TEST_F(ModbusRtuDecoder, ReadCoilsResponseSingleCoil) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x01)
                                                .Write<uint8_t>(1)
                                                .Write<uint8_t>(0b0000'0001)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu, VariantWith<modbus::ReadCoilsResponse>(AllOf(
                             Field(&modbus::ReadCoilsResponse::coils,
                                   ElementsAre(std::byte{0b0000'0001})))));
}

TEST_F(ModbusRtuDecoder, ReadCoilsResponseMultipleCoils) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x01)
                                                .Write<uint8_t>(2)
                                                .Write<uint8_t>(0b1111'0000)
                                                .Write<uint8_t>(0b1010'0101)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu, VariantWith<modbus::ReadCoilsResponse>(
                             Field(&modbus::ReadCoilsResponse::coils,
                                   ElementsAre(std::byte{0b1111'0000},
                                               std::byte{0b1010'0101}))));
}

TEST_F(ModbusRtuDecoder, ReadDiscreteInputsRequest) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x12)
                                               .Write<uint8_t>(0x02)
                                               .Write<uint16_t>(0x0025)
                                               .Write<uint16_t>(0x0017)
                                               .WriteCrc16()
                                               .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Pdu        = ReadDiscreteInputsRequest;
  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x12);
  ASSERT_THAT(frame.pdu,
              VariantWith<Pdu>(AllOf(Field(&Pdu::starting_addr, 0x0025),
                                     Field(&Pdu::num_inputs, 0x0017))));
}

TEST_F(ModbusRtuDecoder, ReadDiscreteInputsResponse) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x02)
                                                .Write<uint8_t>(2)
                                                .Write<uint8_t>(0b1111'0000)
                                                .Write<uint8_t>(0b1010'0101)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Pdu        = ReadDiscreteInputsResponse;
  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu,
              VariantWith<Pdu>(
                  Field(&Pdu::inputs, ElementsAre(std::byte{0b1111'0000},
                                                  std::byte{0b1010'0101}))));
}

TEST_F(ModbusRtuDecoder, ReadHoldingRegistersRequest) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x12)
                                               .Write<uint8_t>(0x03)
                                               .Write<uint16_t>(0x0A04)
                                               .Write<uint16_t>(0x0005)
                                               .WriteCrc16()
                                               .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Pdu        = ReadHoldingRegistersRequest;
  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x12);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(AllOf(
                             Field(&Pdu::starting_addr, 0x0A04),
                             Field(&Pdu::num_holding_registers, 0x0005))));
}

TEST_F(ModbusRtuDecoder, ReadHoldingRegistersResponseSingleRegister) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x03)
                                                .Write<uint8_t>(2)
                                                .Write<uint16_t>(0xBAAB)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Frame      = ReadHoldingRegistersResponse<FrameVariant::Decode>;
  const auto frame = decode_result.value();

  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu,
              VariantWith<Frame>(
                  Field(&Frame::registers, ElementsAre(uint16_t{0xBAAB}))));
}

TEST_F(ModbusRtuDecoder, ReadHoldingRegistersResponseMultipleRegisters) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x03)
                                                .Write<uint8_t>(6)
                                                .Write<uint16_t>(0xBAAB)
                                                .Write<uint16_t>(0x1234)
                                                .Write<uint16_t>(0xEBAF)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Pdu        = ReadHoldingRegistersResponse<FrameVariant::Decode>;
  const auto frame = decode_result.value();

  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(Field(
                             &Pdu::registers,
                             ElementsAre(uint16_t{0xBAAB}, uint16_t{0x1234},
                                         uint16_t{0xEBAF}))));
}

TEST_F(ModbusRtuDecoder, ReadInputRegistersRequest) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x12)
                                               .Write<uint8_t>(0x04)
                                               .Write<uint16_t>(0xAA00)
                                               .Write<uint16_t>(0x0100)
                                               .WriteCrc16()
                                               .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Pdu        = ReadInputRegistersRequest;
  const auto frame = decode_result.value();
  ASSERT_EQ(frame.address, 0x12);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(
                             AllOf(Field(&Pdu::starting_addr, 0xAA00),
                                   Field(&Pdu::num_input_registers, 0x0100))));
}

TEST_F(ModbusRtuDecoder, ReadInputRegistersResponse) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x04)
                                                .Write<uint8_t>(4)
                                                .Write<uint16_t>(0xBAAB)
                                                .Write<uint16_t>(0x1234)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  using Pdu        = ReadInputRegistersResponse<FrameVariant::Decode>;
  const auto frame = decode_result.value();

  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(Field(
                             &Pdu::registers,
                             ElementsAre(uint16_t{0xBAAB}, uint16_t{0x1234}))));
}

TEST_F(ModbusRtuDecoder, WriteSingleCoilRequest) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x10)
                                               .Write<uint8_t>(0x05)
                                               .Write<uint16_t>(0x0020)
                                               .Write<uint16_t>(0xFF00)
                                               .WriteCrc16()
                                               .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  using Pdu        = WriteSingleCoilRequest;
  ASSERT_EQ(frame.address, 0x10);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(AllOf(
                             Field(&Pdu::coil_addr, 0x0020),
                             Field(&Pdu::new_state, CoilState::Enabled))));
}

TEST_F(ModbusRtuDecoder, WriteSingleCoilResponse) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x10)
                                                .Write<uint8_t>(0x05)
                                                .Write<uint16_t>(0x0030)
                                                .Write<uint16_t>(0xFF00)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  using Pdu        = WriteSingleCoilResponse;
  ASSERT_EQ(frame.address, 0x10);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(AllOf(
                             Field(&Pdu::coil_addr, 0x0030),
                             Field(&Pdu::new_state, CoilState::Enabled))));
}

TEST_F(ModbusRtuDecoder, WriteMultipleCoilsRequest) {
  const auto decode_result = DecodeRequest(FrameBuilder()
                                               .Write<uint8_t>(0x06)
                                               .Write<uint8_t>(0x0F)
                                               .Write<uint16_t>(0x0020)
                                               .Write<uint16_t>(13)
                                               .Write<uint8_t>(2)
                                               .Write<uint8_t>(0b1111'0000)
                                               .Write<uint8_t>(0b1010'0101)
                                               .WriteCrc16()
                                               .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  using Pdu        = WriteMultipleCoilsRequest;
  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu,
              VariantWith<Pdu>(AllOf(
                  Field(&Pdu::start_addr, 0x0020), Field(&Pdu::num_coils, 13),
                  Field(&Pdu::values, ElementsAre(std::byte{0b1111'0000},
                                                  std::byte{0b1010'0101})))));
}

TEST_F(ModbusRtuDecoder, WriteMultipleCoilsResponse) {
  const auto decode_result = DecodeResponse(FrameBuilder()
                                                .Write<uint8_t>(0x06)
                                                .Write<uint8_t>(0x0F)
                                                .Write<uint16_t>(0x0030)
                                                .Write<uint16_t>(13)
                                                .WriteCrc16()
                                                .Bytes());

  ASSERT_TRUE(decode_result.has_value());

  const auto frame = decode_result.value();
  using Pdu        = WriteMultipleCoilsResponse;
  ASSERT_EQ(frame.address, 0x06);
  ASSERT_THAT(frame.pdu, VariantWith<Pdu>(AllOf(Field(&Pdu::start_addr, 0x0030),
                                                Field(&Pdu::num_coils, 13))));
}
