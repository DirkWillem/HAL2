#pragma once

#include <span>
#include <string_view>

#include <gmock/gmock.h>

#include <hal/uart.h>

#include "fake_uart.h"

namespace doubles {

template <hal::UartFlowControl FC = hal::UartFlowControl::None>
class MockBlockingUart {
 public:
  static constexpr auto FlowControl   = FC;
  static constexpr auto OperatingMode = hal::UartOperatingMode::Poll;

  MOCK_METHOD(void, WriteBlocking, (std::string_view), ());
  MOCK_METHOD(void, WriteBlocking, (std::span<const std::byte>), ());

  MOCK_METHOD(void, ReceiveBlocking, (std::span<std::byte>), ());
};

class UartRxSpy {
 public:
  MOCK_METHOD(void, Receive, (std::span<std::byte>), ());
};

template <hal::UartFlowControl FC = hal::UartFlowControl::None>
class MockAsyncUart {
 public:
  static constexpr auto FlowControl   = FC;
  static constexpr auto OperatingMode = hal::UartOperatingMode::Dma;

  MOCK_METHOD(void, Write, (std::string_view), ());
  MOCK_METHOD(void, Write, (std::span<const std::byte>), ());

  MOCK_METHOD(void, Receive, (std::span<std::byte>), ());

  MOCK_METHOD(void, UartReceiveCallback, (std::span<std::byte>), ());
  MOCK_METHOD(void, RegisterUartReceiveCallback,
              (hal::Callback<std::span<std::byte>>&), ());

  void RedirectRxToFake() {
    ON_CALL(*this, Receive).WillByDefault([this](auto buf) {
      rx_spy.Receive(buf);
      fake.Receive(buf);
    });

    // Receive callback
    ON_CALL(*this, UartReceiveCallback).WillByDefault([this](auto data) {
      fake.ReceiveCallback(data);
    });
    ON_CALL(*this, RegisterUartReceiveCallback)
        .WillByDefault([this](auto& callback) {
          fake.RegisterReceiveCallback(callback);   //
        });
  }

  FakeAsyncUart                fake;
  testing::NiceMock<UartRxSpy> rx_spy;
};

static_assert(hal::BlockingUart<MockBlockingUart<hal::UartFlowControl::None>>);
static_assert(hal::AsyncUart<MockAsyncUart<>>);

}   // namespace doubles