#include <memory>

#include <gtest/gtest.h>

#include <vrpc/uart/vrpc_uart.h>

#include <helpers/buffer_builder.h>

#include <hello_world_service.h>

#include <doubles/hal/mock_system.h>
#include <doubles/hal/mock_uart.h>

using namespace testing;

struct ServiceA {
  static constexpr uint32_t ServiceId = 0x100;

  static constexpr std::size_t MinBufferSize() noexcept { return 128; }

  MOCK_METHOD(vrpc::uart::HandleResult, HandleCommand,
              (uint32_t, std::span<const std::byte>, std::span<std::byte>,
               hal::Callback<>&));
};

struct ServiceB {
  static constexpr uint32_t ServiceId = 0x101;

  static constexpr std::size_t MinBufferSize() noexcept { return 128; }

  MOCK_METHOD(vrpc::uart::HandleResult, HandleCommand,
              (uint32_t, std::span<const std::byte>, std::span<std::byte>,
               hal::Callback<>&));
};

class VrpcUartServer : public Test {
 public:
  void SetUp() final {
    uart = std::make_unique<NiceMock<doubles::MockAsyncUart<>>>();
    uart->RedirectRxToFake();

    svc_a = std::make_unique<ServiceA>();
    svc_b = std::make_unique<ServiceB>();

    vrpc_uart = std::make_unique<vrpc::uart::VrpcUartServer<
        doubles::MockAsyncUart<>, doubles::MockCriticalSectionInterface,
        ServiceA, ServiceB>>(*uart, *svc_a, *svc_b);
  }

 protected:
  void SendRxData(std::function<void(helpers::BufferBuilder&)> build) {
    std::array<std::byte, 1024> buffer{};
    helpers::BufferBuilder      bb{buffer};
    build(bb);

    uart->fake.FakeReceive(bb.Bytes());
  }

  std::unique_ptr<ServiceA> svc_a{};
  std::unique_ptr<ServiceB> svc_b{};

  std::unique_ptr<doubles::MockAsyncUart<>> uart{};
  std::unique_ptr<vrpc::uart::VrpcUartServer<doubles::MockAsyncUart<>,
                                       doubles::MockCriticalSectionInterface,
                                       ServiceA, ServiceB>>
      vrpc_uart{};
};

TEST_F(VrpcUartServer, HasNoPendingRequestByDefault) {
  ASSERT_FALSE(vrpc_uart->has_pending_requests());
}

TEST_F(VrpcUartServer, ReceivePartialCommand) {
  constexpr auto WrittenBytes = 12;

  EXPECT_CALL(uart->rx_spy, Receive(SizeIs(vrpc_uart->BufSize - WrittenBytes)));

  // Send Rx data
  SendRxData([](auto& bb) {
    helloworld::HelloWorldRequest request{};
    bb.Write('V')                          // Start of frame
        .Write('C')                        // Frame type
        .template Write<uint32_t>(0x100)   // Service ID
        .template Write<uint32_t>(0x10)    // Method ID
        .template Write<uint16_t>(0x00);   // Partial request ID
  });

  ASSERT_FALSE(vrpc_uart->has_pending_requests());
}

TEST_F(VrpcUartServer, ReceiveCommand) {
  EXPECT_CALL(uart->rx_spy, Receive(SizeIs(vrpc_uart->BufSize)));

  SendRxData([](auto& bb) {
    helloworld::HelloWorldRequest request{};
    bb.Write('V')                          // Start of frame
        .Write('C')                        // Frame type
        .template Write<uint32_t>(0x100)   // Service ID
        .template Write<uint32_t>(0x10)    // Method ID
        .template Write<uint32_t>(3)       // Request ID
        .WriteProto(request,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly);
  });

  ASSERT_TRUE(vrpc_uart->has_pending_requests());
}

TEST_F(VrpcUartServer, ReceiveTwoCommandsAtOnce) {
  // No receive should be called, because there are no buffers left
  EXPECT_CALL(uart->rx_spy, Receive).Times(0);

  SendRxData([](auto& bb) {
    helloworld::HelloWorldRequest request{};
    bb.Write('V')                          // Start of frame
        .Write('C')                        // Frame type
        .template Write<uint32_t>(0x100)   // Service ID
        .template Write<uint32_t>(0x10)    // Method ID
        .template Write<uint32_t>(3)       // Request ID
        .WriteProto(request,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly);

    const auto msg1_bytes = bb.BytesWritten();

    bb.Write('V')                          // Start of frame
        .Write('C')                        // Frame type
        .template Write<uint32_t>(0x101)   // Service ID
        .template Write<uint32_t>(0x20)    // Method ID
        .template Write<uint32_t>(4)       // Request ID
        .WriteProto(request,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly, msg1_bytes);
  });

  ASSERT_TRUE(vrpc_uart->has_pending_requests());
}

TEST_F(VrpcUartServer, ReceiveTwoCommandsConsecutively) {
  // No receive should be called, because there are no buffers left
  EXPECT_CALL(uart->rx_spy, Receive).Times(1);

  SendRxData([](auto& bb) {
    helloworld::HelloWorldRequest request{};
    bb.Write('V')                          // Start of frame
        .Write('C')                        // Frame type
        .template Write<uint32_t>(0x100)   // Service ID
        .template Write<uint32_t>(0x10)    // Method ID
        .template Write<uint32_t>(3)       // Request ID
        .WriteProto(request,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly);
  });
  SendRxData([](auto& bb) {
    helloworld::HelloWorldRequest request{};
    bb.Write('V')                          // Start of frame
        .Write('C')                        // Frame type
        .template Write<uint32_t>(0x100)   // Service ID
        .template Write<uint32_t>(0x10)    // Method ID
        .template Write<uint32_t>(3)       // Request ID
        .WriteProto(request,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly);
  });

  ASSERT_TRUE(vrpc_uart->has_pending_requests());
}

TEST_F(VrpcUartServer, InvokeService) {
  EXPECT_CALL(*svc_a, HandleCommand(0x10, _, SizeIs(128), _));

  SendRxData([](auto& bb) {
    helloworld::HelloWorldRequest request{};
    bb.Write('V')                                        // Start of frame
        .Write('C')                                      // Frame type
        .template Write<uint32_t>(ServiceA::ServiceId)   // Service ID
        .template Write<uint32_t>(0x10)                  // Method ID
        .template Write<uint32_t>(3)                     // Request ID
        .WriteProto(request,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly);
  });

  vrpc_uart->HandlePendingRequests();
  ASSERT_FALSE(vrpc_uart->has_pending_requests());
}
