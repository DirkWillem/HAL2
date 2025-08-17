module;

#include <bit>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <ratio>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

export module hal.sil:uart;

import hstd;

import hal.abstract;

import :scheduler;

import rtos.concepts;

namespace sil {

export class Uart;

export class Uart : public ::hal::UsedPeripheral {
 public:
  Uart()          = default;
  virtual ~Uart() = default;

  /**
   * Returns the name of the UART
   * @return Name of the UART
   */
  virtual const std::string& GetName() const& = 0;

  /**
   * Simulates a receive data event at the given time
   * @param timestamp Timestamp at which the data should be received
   * @param rx_data Data to receive
   */
  virtual void SimulateRx(TimePointUs                timestamp,
                          std::span<const std::byte> rx_data) = 0;

  /**
   * Sets the UART transmit callback
   * @param callback Callback to be invoked upon UART transmission
   */
  virtual void
  SetTxCallback(std::function<void(std::span<const std::byte>)> callback) = 0;

  /**
   * Clears the UART transmit callback
   */
  virtual void ClearTxCallback() = 0;
};

export template <rtos::concepts::Rtos OS>
class RtosUart final : public Uart {
 public:
  explicit RtosUart(Scheduler& sched, std::string_view name,
                    unsigned          baud      = 115'200,
                    hal::UartParity   parity    = hal::UartParity::None,
                    hal::UartStopBits stop_bits = hal::UartStopBits::One)
      : sched{sched}
      , rx_evt{sched.RegisterExternalEvent()}
      , tx_evt{sched.RegisterExternalEvent()}
      , name{name}
      , baud{baud}
      , parity{parity}
      , stop_bits{stop_bits} {}

  bool Write(std::span<const std::byte> data, hstd::Duration auto timeout) {
    Write(data, event_group, TxDoneBit);

    return event_group.Wait(TxDoneBit, timeout).has_value();
  }

  bool Write(std::string_view data, hstd::Duration auto timeout) {
    Write(data, event_group, TxDoneBit);

    return event_group.Wait(TxDoneBit, timeout).has_value();
  }

  void Write(std::string_view data, OS::EventGroup& eg, uint32_t bitmask) {
    Write(hstd::ReinterpretSpan<std::byte>(
              std::span<const char>{data.data(), data.size()}),
          eg, bitmask);
  }

  void Write(std::span<const std::byte> data, OS::EventGroup& eg,
             uint32_t bitmask) {
    // Simulate no-op if still transmitting
    if (tx_evt.HasPendingAction()) {
      return;
    }

    tx_event_group = std::make_tuple(&eg, bitmask);
    pending_tx_buf.resize(data.size());
    std::ranges::copy(data, pending_tx_buf.begin());

    tx_evt.RegisterPendingAction(sched.Now() + TransmissionTime(data.size()),
                                 [this] {
                                   // Invoke callback
                                   if (tx_callback.has_value()) {
                                     (*tx_callback)(pending_tx_buf);
                                   }

                                   // Set transmitted bit
                                   auto& [eg, bitmask] = tx_event_group;
                                   if (eg != nullptr) {
                                     eg->SetBitsFromInterrupt(bitmask);
                                   }
                                 });
  }

  std::optional<std::span<std::byte>> Receive(std::span<std::byte> into,
                                              hstd::Duration auto  timeout) {
    Receive(into, event_group, RxDoneBit);

    if (event_group.Wait(RxDoneBit, timeout).has_value()) {
      return rx_buf;
    }

    return {};
  }

  void Receive(std::span<std::byte> into, typename OS::EventGroup& event_group,
               uint32_t bitmask) {
    rx_buf         = into;
    rx_event_group = std::make_tuple(&event_group, bitmask);
  }

  /**
   * Simulates a receive on the UART at a given timestamp
   * @param timestamp Timestamp at which the receive should happen
   * @param rx_data Data to receive
   */
  void SimulateRx(TimePointUs                timestamp,
                  std::span<const std::byte> rx_data) final {
    if (timestamp < sched.Now()) {
      throw std::runtime_error{"Cannot simulate receive event in the past"};
    }

    if (rx_evt.HasPendingAction()) {
      throw std::runtime_error{std::format(
          "A simulated RX on UART {} is still pending, cannot add new event",
          name)};
    }

    pending_rx_buf.resize(rx_data.size());
    std::ranges::copy(rx_data, pending_rx_buf.begin());

    rx_evt.RegisterPendingAction(timestamp, [this] {
      if (rx_buf.has_value()) {
        const auto size = std::min(rx_buf->size(), pending_rx_buf.size());
        std::ranges::copy(std::span{pending_rx_buf}.subspan(0, size),
                          rx_buf->begin());
        rx_buf = rx_buf->subspan(0, size);
      }

      auto& [eg, bitmask] = rx_event_group;
      if (eg != nullptr) {
        eg->SetBitsFromInterrupt(bitmask);
      }
    });
  }

  /**
   * Sets the UART transmit callback
   * @param callback Callback to be invoked upon UART transmission
   */
  void SetTxCallback(
      std::function<void(std::span<const std::byte>)> callback) final {
    tx_callback = callback;
  }

  /**
   * Clears the UART transmit callback
   */
  void ClearTxCallback() final { tx_callback = {}; }

  const std::string& GetName() const& final { return name; }

 private:
  /**
   * Calculates the approximate transmission time for a given amount of bytes
   * @param n_bytes Amount of bytes to get the transmission time for
   * @return Approximate transmission time
   */
  DurationUs TransmissionTime(std::size_t n_bytes) const {
    auto BaudPeriodUs =
        static_cast<double>(std::micro::den) / static_cast<double>(baud);
    auto bit_changes = 8.0;

    switch (parity) {
    case hal::UartParity::Even: [[fallthrough]];
    case hal::UartParity::Odd: bit_changes += 1.0;
    case hal::UartParity::None: break;
    }

    switch (stop_bits) {
    case hal::UartStopBits::Half: bit_changes += 0.5;
    case hal::UartStopBits::One: bit_changes += 1.0;
    case hal::UartStopBits::OneAndHalf: bit_changes += 1.5; ;
    case hal::UartStopBits::Two: bit_changes += 2.0;
    }

    const auto us =
        static_cast<uint64_t>(std::ceil(bit_changes * n_bytes * BaudPeriodUs));
    return DurationUs{us};
  }

  Scheduler& sched;

  ExternalEventItem& rx_evt;
  ExternalEventItem& tx_evt;

  std::string       name;
  unsigned          baud;
  hal::UartParity   parity;
  hal::UartStopBits stop_bits;

  static constexpr uint32_t TxDoneBit = (0b1U << 0U);
  static constexpr uint32_t RxDoneBit = (0b1U << 1U);

  OS::EventGroup                                 event_group{};
  std::tuple<typename OS::EventGroup*, uint32_t> tx_event_group{};
  std::tuple<typename OS::EventGroup*, uint32_t> rx_event_group{};

  std::vector<std::byte>              pending_rx_buf{};
  std::vector<std::byte>              pending_tx_buf{};
  std::optional<std::span<std::byte>> rx_buf{};

  std::optional<std::function<void(std::span<const std::byte>)>> tx_callback;
};

}   // namespace sil

extern "C" {}