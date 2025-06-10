#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import modbus.encoding;
import modbus.encoding.rtu;

import testing.helpers;

using namespace ::testing;

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

    req_buffer_builder =
        std::make_unique<helpers::BufferBuilder<std::endian::big>>(
            req_buffer,
            helpers::BufferBuilderSettings{.default_crc16_poly = 0xA001,
                                           .default_crc16_init = 0xFFFF});
    return *req_buffer_builder;
  }

 protected:
  std::array<std::byte, 256>                                req_buffer{};
  std::unique_ptr<helpers::BufferBuilder<std::endian::big>> req_buffer_builder{
      nullptr};
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
  ASSERT_THAT(frame.payload,
              VariantWith<modbus::ErrorResponse>(
                  AllOf(Field(&modbus::ErrorResponse::function_code, 0x81),
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
  ASSERT_THAT(frame.payload,
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
  ASSERT_THAT(frame.payload, VariantWith<modbus::ReadCoilsResponse>(AllOf(
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
  ASSERT_THAT(
      frame.payload,
      VariantWith<modbus::ReadCoilsResponse>(AllOf(
          Field(&modbus::ReadCoilsResponse::coils,
                ElementsAre(std::byte{0b1111'0000}, std::byte{0b1010'0101})))));
}
