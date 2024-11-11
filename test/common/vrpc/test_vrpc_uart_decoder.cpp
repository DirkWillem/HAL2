#include <memory>

#include <gtest/gtest.h>

#include <vrpc/uart/vrpc_uart_decoder.h>

#include <helpers/buffer_builder.h>

#include <common/vrpc/generated/hello_world_service.pb.h>

using namespace testing;

TEST(VrpcUartDecode, DecodeEmpty) {
  std::array<std::byte, 128> data{};
  vrpc::UartDecoder          dec{data};
  const auto                 result = dec.ConsumeBytes(0);

  ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
  ASSERT_TRUE(dec.buffer_empty());
}

/*
TEST(VrpcUartDecode, DecodeRequestFrame) {
  // Build input buffer
  helloworld_HelloWorldRequest request{};
  std::strcpy(request.name, "Testing");

  std::array<std::byte, 128> data{};
  helpers::BufferBuilder     bb{data};
  bb.Write('V')                 // Start of frame
      .Write('C')               // Frame type
      .Write<uint32_t>(0x100)   // Service ID
      .Write<uint32_t>(0x10)    // Method ID
      .Write<uint32_t>(3)       // Request ID
      .WriteProto(request, [&bb](auto size) { bb.Write<uint32_t>(size); }).WriteCrc16(vrpc::CrcPoly);

  // Decode data
  vrpc::UartDecoder dec{data};
  const auto        result = dec.ConsumeBytes(bb.BytesWritten());
  ASSERT_TRUE(dec.buffer_empty());

  // Validate result
  ASSERT_TRUE(std::holds_alternative<vrpc::CommandRequestFrameRef>(result));

  const auto& req = std::get<vrpc::CommandRequestFrameRef>(result).get();
  ASSERT_EQ(req.service_id, 0x100);
  ASSERT_EQ(req.command_id, 0x10);
  ASSERT_EQ(req.request_id, 3);

  const auto msg =
      helpers::DecodeProto<helloworld_HelloWorldRequest>(req.payload);
  ASSERT_STREQ(msg.name, "Testing");
}

TEST(VrpcUartDecode, DecodeEmptyRequestFrame) {
  // Build input buffer
  helloworld_HelloWorldRequest request{};

  std::array<std::byte, 128> data{};
  helpers::BufferBuilder     bb{data};
  bb.Write('V')                 // Start of frame
      .Write('C')               // Frame type
      .Write<uint32_t>(0x100)   // Service ID
      .Write<uint32_t>(0x10)    // Method ID
      .Write<uint32_t>(3)       // Request ID
      .WriteProto(request, [&bb](auto size) { bb.Write<uint32_t>(size); })
      .WriteCrc16(vrpc::CrcPoly);

  // Decode data
  vrpc::UartDecoder dec{data};
  const auto        result = dec.ConsumeBytes(bb.BytesWritten());
  ASSERT_TRUE(dec.buffer_empty());

  // Validate result
  ASSERT_TRUE(std::holds_alternative<vrpc::CommandRequestFrameRef>(result));

  const auto& req = std::get<vrpc::CommandRequestFrameRef>(result).get();
  ASSERT_EQ(req.service_id, 0x100);
  ASSERT_EQ(req.command_id, 0x10);
  ASSERT_EQ(req.request_id, 3);
  ASSERT_EQ(req.payload.size(), 0);
}

TEST(VrpcUartDecode, DecodeRequestFrameInParts) {
  // Build input buffer
  helloworld_HelloWorldRequest request{};
  std::strcpy(request.name, "Testing");

  std::array<std::byte, 128> data{};
  helpers::BufferBuilder     bb{data};
  bb.Write('V')                 // Start of frame
      .Write('C')               // Frame type
      .Write<uint32_t>(0x100)   // Service ID
      .Write<uint32_t>(0x10)    // Method ID
      .Write<uint32_t>(3)       // Request ID
      .WriteProto(request, [&bb](auto size) { bb.Write<uint32_t>(size); })
      .WriteCrc16(vrpc::CrcPoly);

  // Decode data in parts
  vrpc::UartDecoder dec{data};
  const auto        result1 = dec.ConsumeBytes(bb.BytesWritten() - 12);
  ASSERT_FALSE(dec.buffer_empty());
  ASSERT_TRUE(dec.HasPartialCommand());
  const auto result2 = dec.ConsumeBytes(8);
  ASSERT_FALSE(dec.buffer_empty());
  ASSERT_TRUE(dec.HasPartialCommand());
  const auto result3 = dec.ConsumeBytes(4);
  ASSERT_TRUE(dec.buffer_empty());
  ASSERT_FALSE(dec.HasPartialCommand());

  // Validate result
  ASSERT_TRUE(std::holds_alternative<std::monostate>(result1));
  ASSERT_TRUE(std::holds_alternative<std::monostate>(result2));
  ASSERT_TRUE(std::holds_alternative<vrpc::CommandRequestFrameRef>(result3));

  const auto& req = std::get<vrpc::CommandRequestFrameRef>(result3).get();
  ASSERT_EQ(req.service_id, 0x100);
  ASSERT_EQ(req.command_id, 0x10);
  ASSERT_EQ(req.request_id, 3);

  const auto msg =
      helpers::DecodeProto<helloworld_HelloWorldRequest>(req.payload);
  ASSERT_STREQ(msg.name, "Testing");
}
 */