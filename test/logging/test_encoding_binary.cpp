#include <array>
#include <bit>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

import logging;

import hal2.testing.helpers;

using namespace ::testing;
using namespace ::hal2::testing::helpers;

using HelloMsg = logging::Message<"Hello World!">;
using CountMsg = logging::Message<"Count={}, stdev={}", uint32_t, float>;

using MyModule = logging::Module<0xABCD, HelloMsg, CountMsg>;

class BinaryEncoding : public Test {
 public:
  void SetUp() override {
    std::fill(buffer.begin(), buffer.end(), std::byte{0});
    std::fill(req_buffer.begin(), req_buffer.end(), std::byte{0});
  }

  void TearDown() override { req_buffer_builder = nullptr; }

  auto& MsgBuilder() & {
    if (req_buffer_builder != nullptr) {
      throw std::runtime_error{
          "Can only create one check buffer per test case"};
    }

    req_buffer_builder = std::make_unique<BufferBuilder<std::endian::native>>(
        req_buffer, BufferBuilderSettings{.default_crc16_poly = 0xA001,
                                          .default_crc16_init = 0x0000});
    return *req_buffer_builder;
  }

 protected:
  std::array<std::byte, 64> buffer{};

 private:
  std::array<std::byte, 64>                           req_buffer{};
  std::unique_ptr<BufferBuilder<std::endian::native>> req_buffer_builder{
      nullptr};
};

TEST_F(BinaryEncoding, EncodeEmptyMessage) {
  // Create expected message
  const auto expected = MsgBuilder()
                            .Write<uint8_t>('L')       // Start byte
                            .Write<uint32_t>(10'000)   // Timestamp
                            .Write<uint8_t>(30)        // Log Level - Info
                            .Write<uint16_t>(0xABCD)   // Module ID
                            .Write<uint8_t>(1)         // Message ID 1
                            .Write<uint8_t>(0)         // 0 data bytes
                            .WriteCrc16()              // CRC
                            .Bytes();

  // Encode message
  const auto encode_result =
      logging::encoding::Binary::Encode<MyModule, HelloMsg>(
          10'000, logging::Level::Info, HelloMsg{}, buffer);

  ASSERT_THAT(encode_result, ElementsAreArray(expected));
}

TEST_F(BinaryEncoding, EncodeMessageWithData) {
  const auto f = 12.34F;
  // Create expected message
  const auto expected = MsgBuilder()
                            .Write<uint8_t>('L')       // Start byte
                            .Write<uint32_t>(10'000)   // Timestamp
                            .Write<uint8_t>(50)        // Log Level - Error
                            .Write<uint16_t>(0xABCD)   // Module ID
                            .Write<uint8_t>(2)         // Message ID 2
                            .Write<uint8_t>(8)         // 8 data bytes
                            .Write<uint32_t>(123)      // Argument 1
                            .Write(f)                  // Argument 2
                            .WriteCrc16()              // CRC
                            .Bytes();

  // Encode message
  const auto encode_result = logging::encoding::Binary::Encode<MyModule>(
      10'000, logging::Level::Error, CountMsg{123, f}, buffer);

  ASSERT_THAT(encode_result, ElementsAreArray(expected));
}