#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import modbus.encoding;
import modbus.encoding.rtu;

import testing.helpers;

class ModbusRtuEncoder : public ::testing::Test {
 public:
  void SetUp() override {
    std::fill(buffer.begin(), buffer.end(), std::byte{0});
    std::fill(check_buffer.begin(), buffer.end(), std::byte{0});
  }

  void TearDown() override { check_buffer_builder = nullptr; }

  std::span<const std::byte>
  EncodeRequest(modbus::RequestFrame<modbus::FrameVariant::Encode> frame) {
    auto encoder =
        modbus::encoding::rtu::Encoding::Encoder{frame.address, buffer};
    return std::visit(encoder, frame.payload);
  }

  std::span<const std::byte>
  EncodeResponse(modbus::ResponseFrame<modbus::FrameVariant::Encode> frame) {
    auto encoder =
        modbus::encoding::rtu::Encoding::Encoder{frame.address, buffer};
    return std::visit(encoder, frame.payload);
  }

  auto& CheckBuilder() & {
    if (check_buffer_builder != nullptr) {
      throw std::runtime_error{
          "Can only create one check buffer per test case"};
    }

    check_buffer_builder =
        std::make_unique<helpers::BufferBuilder<std::endian::big>>(
            check_buffer,
            helpers::BufferBuilderSettings{.default_crc16_poly = 0xA001,
                                           .default_crc16_init = 0xFFFF});
    return *check_buffer_builder;
  }

 protected:
  std::array<std::byte, 256> buffer{};

  std::array<std::byte, 256> check_buffer{};
  std::unique_ptr<helpers::BufferBuilder<std::endian::big>>
      check_buffer_builder{nullptr};
};

TEST_F(ModbusRtuEncoder, ErrorResponse) {
  const auto encoded_frame = EncodeResponse({
      .payload =
          modbus::MakeErrorResponse(modbus::FunctionCode::ReadCoils,
                                    modbus::ExceptionCode::IllegalDataValue),
      .address = 0xEE,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0xEE)
                                       .Write<uint8_t>(0x81)
                                       .Write<uint8_t>(0x03)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ::testing::ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadCoilsRequest) {
  const auto encoded_frame = EncodeRequest({
      .payload =
          modbus::ReadCoilsRequest{
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

  ASSERT_THAT(encoded_frame, ::testing::ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadCoilsResponseSingleCoil) {
  std::array<std::byte, 1> coils{std::byte{0b0000'0001}};

  const auto encoded_frame = EncodeResponse({
      .payload = modbus::ReadCoilsResponse{.coils = coils},
      .address = 0x06,
  });

  const auto encoded_frame_check = CheckBuilder()
                                       .Write<uint8_t>(0x06)
                                       .Write<uint8_t>(0x01)
                                       .Write<uint8_t>(1)
                                       .Write<uint8_t>(0b0000'0001)
                                       .WriteCrc16()
                                       .Bytes();

  ASSERT_THAT(encoded_frame, ::testing::ElementsAreArray(encoded_frame_check));
}

TEST_F(ModbusRtuEncoder, ReadCoilsResponseMultipleCoils) {
  std::array<std::byte, 2> coils{std::byte{0b1111'0000},
                                 std::byte{0b1010'0101}};

  const auto encoded_frame = EncodeResponse({
      .payload = modbus::ReadCoilsResponse{.coils = coils},
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

  ASSERT_THAT(encoded_frame, ::testing::ElementsAreArray(encoded_frame_check));
}