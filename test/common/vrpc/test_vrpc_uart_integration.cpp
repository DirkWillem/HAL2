#include <memory>

#include <gtest/gtest.h>

#include <vrpc/uart/vrpc_uart.h>

#include <helpers/buffer_builder.h>

#include <common/vrpc/generated/hello_world_service.h>

#include <doubles/hal/mock_system.h>
#include <doubles/hal/mock_uart.h>

#include <calculator_service.h>
#include <calculator_service_uart.h>

using namespace testing;

class CalculatorServiceImpl {
 public:
  MOCK_METHOD(void, Calculate,
              (const calculator::CalculatorRequest&,
               calculator::CalculatorResponse&),
              ());
};

class VrpcUartIntegration : public Test {
 public:
  void SetUp() final {
    uart = std::make_unique<NiceMock<doubles::MockAsyncUart<>>>();
    uart->RedirectRxToFake();

    calculator_service      = std::make_unique<CalculatorServiceImpl>();
    uart_calculator_service = std::make_unique<
        calculator::uart::UartCalculatorService<CalculatorServiceImpl>>(
        *calculator_service);

    vrpc_uart = std::make_unique<vrpc::uart::VrpcUart<
        doubles::MockAsyncUart<>, doubles::MockCriticalSectionInterface,
        calculator::uart::UartCalculatorService<CalculatorServiceImpl>>>(
        *uart, *uart_calculator_service);
  }

 protected:
  void SendRxData(std::function<void(helpers::BufferBuilder&)> build) {
    std::array<std::byte, 1024> buffer{};
    helpers::BufferBuilder      bb{buffer};
    build(bb);

    uart->fake.FakeReceive(bb.Bytes());
  }

  std::unique_ptr<CalculatorServiceImpl> calculator_service{};
  std::unique_ptr<
      calculator::uart::UartCalculatorService<CalculatorServiceImpl>>
      uart_calculator_service{};

  std::unique_ptr<doubles::MockAsyncUart<>> uart{};
  std::unique_ptr<vrpc::uart::VrpcUart<
      doubles::MockAsyncUart<>, doubles::MockCriticalSectionInterface,
      calculator::uart::UartCalculatorService<CalculatorServiceImpl>>>
      vrpc_uart{};
};

TEST_F(VrpcUartIntegration, HasNoPendingRequestByDefault) {
  EXPECT_CALL(
      *calculator_service,
      Calculate(FieldsAre(1, 3, calculator_CalculatorOp_CALCULATOR_OP_ADD), _))
      .WillOnce([](const auto& req, auto& res) { res.result = 10; });

  // Write data to buffer and handle commands
  SendRxData([](auto& bb) {
    calculator::CalculatorRequest req{
        .lhs = 1,
        .rhs = 3,
        .op  = static_cast<_calculator_CalculatorOp>(
            calculator::CalculatorOp::Add),
    };
    bb.Write('V')     // Start of frame
        .Write('C')   // Frame type
        .template Write<uint32_t>(
            calculator::CalculatorServiceId)   // Service ID
        .template Write<uint32_t>(0x01)        // Method ID
        .template Write<uint32_t>(3)           // Request ID
        .WriteProto(req,
                    [&bb](auto size) { bb.template Write<uint32_t>(size); })
        .WriteCrc16(vrpc::CrcPoly);
  });

  vrpc_uart->HandlePendingRequests();
}
