#pragma once

#include <memory>
#include <optional>
#include <span>

#include <hal/uart.h>

namespace doubles {

class FakeAsyncUart {
  template <hal::UartFlowControl>
  friend class MockAsyncUart;

 public:
  constexpr void FakeReceive(std::span<const std::byte> data) noexcept {
    if (!rx_buf) {
      return;
    }

    const auto rx_size = std::min(data.size(), rx_buf->size());
    std::memcpy(rx_buf->data(), data.data(), rx_size);

    if (rx_callback != nullptr) {
      (*rx_callback)({rx_buf->data(), rx_size});
    }

    rx_buf = {};
  }

 protected:
  constexpr void Receive(std::span<std::byte> into) noexcept { rx_buf = into; }

  constexpr void ReceiveCallback(std::span<std::byte> data) noexcept {
    FakeReceive(data);
  }

  constexpr void RegisterReceiveCallback(
      hal::Callback<std::span<std::byte>>& callback) noexcept {
    rx_callback = &callback;
  }

 private:
  hal::Callback<std::span<std::byte>>* rx_callback{nullptr};
  std::optional<std::span<std::byte>>  rx_buf{};
};

}   // namespace doubles