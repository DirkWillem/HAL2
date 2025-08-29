module;

#include <bit>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <functional>
#include <memory>
#include <print>
#include <ranges>
#include <ratio>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

export module hal.sil:spi;

import hstd;

import hal.abstract;

import :scheduler;

namespace sil {

export class SpiMaster : public ::hal::UsedPeripheral {
 public:
  SpiMaster()          = default;
  virtual ~SpiMaster() = default;

  /**
   * Returns the name of the SPI
   * @return Name of the SPI
   */
  virtual const std::string& GetName() const& = 0;

  /**
   * Simulates a receive data event at the given time
   * @param timestamp Timestamp at which the data should be received
   * @param rx_data Data to receive
   */
  virtual void SimulateMisoData(TimePointUs                timestamp,
                                std::span<const std::byte> rx_data) = 0;

  /**
   * Sets the SPI MISO size hint callback
   * @param callback Callback that is invoked when a MISO data read starts
   */
  virtual void
  SetMisoSizeHintCallback(std::function<void(std::size_t)> callback) = 0;

  /**
   * Clears the SPI MISO size hint callback
   */
  virtual void ClearMisoSizeHintCallback() = 0;

  /**
   * Sets the SPI MOSI callback
   * @param callback Callback to be invoked upon MOSI transmission
   */
  virtual void
  SetMosiCallback(std::function<void(std::span<const std::byte>)> callback) = 0;

  /**
   * Clears the SPI MOSI callback
   */
  virtual void ClearMosiCallback() = 0;
};

/**
 * Blocking SPI master
 * @tparam D Data type
 * @tparam DS Data size in bits
 */
export template <typename D  = uint8_t,
                 unsigned DS = std::numeric_limits<D>::digits>
class BlockingSpiMaster final : public SpiMaster {
 public:
  using Data = D;

  static constexpr auto Mode             = hal::SpiMode::Master;
  static constexpr auto TransmissionType = hal::SpiTransmissionType::FullDuplex;
  static constexpr auto DataSize         = DS;

  explicit BlockingSpiMaster(Scheduler& sched, std::string_view name,
                             unsigned f_clk)
      : sched{sched}
      , name{name}
      , f_clk{f_clk}
      , miso_evt{sched.RegisterExternalEvent()} {}

  ~BlockingSpiMaster() final = default;

  const std::string& GetName() const& final { return name; }

  bool TransmitBlocking(std::span<Data> data, hstd::Duration auto) {
    const auto t0 = sched.Now();

    for (std::size_t i = 0; i < data.size(); ++i) {
      // Simulate byte-for-byte transmission delays
      const auto byte = data[i];
      const auto t    = t0 + TransmissionTime(i + 1);
      if (t != sched.Now()) {
        sched.BlockCurrentThreadUntil(t);
      }

      // Invoke MOSI callback
      if (mosi_callback) {
        (*mosi_callback)(hstd::ReinterpretSpan<std::byte>(std::span{&byte, 1}));
      }
    }

    // Simulated SPI transmit never fails
    return true;
  }

  bool ReceiveBlocking(std::span<Data> into, hstd::Duration auto timeout) {
    const auto timeout_at = sched.Now() + timeout;

    // Clear any possibly previously present data
    miso_buf.clear();

    // Invoke the size hint callback to let the simulated system prepare data
    if (miso_size_hint_callback) {
      (*miso_size_hint_callback)(into.size());
    }

    // Keep on handling events until we either hit the timeout or we have enough
    // data
    const auto t0 = sched.Now();

    while (sched.BlockCurrentThreadUntilNextTimepoint(timeout_at)) {
      if (miso_buf.size() >= into.size()) {
        std::ranges::copy(miso_buf | std::views::take(into.size()),
                          hstd::ReinterpretSpanMut<std::byte>(into).begin());
        miso_buf.erase(miso_buf.begin(), miso_buf.begin() + into.size());
        return true;
      }
    }

    // Timeout
    return false;
  }

  void SimulateMisoData(TimePointUs                timestamp,
                        std::span<const std::byte> rx_data) final {
    if (miso_evt.HasPendingAction()) {
      throw std::runtime_error{
          "There is already a pending MISO event, cannot register a new event "
          "before the previous completed"};
    }

    pending_miso_buf.reserve(rx_data.size());
    std::ranges::copy(rx_data, std::back_inserter(pending_miso_buf));

    miso_evt.RegisterPendingAction(timestamp, [this] {
      // Append the data to the MISO buffer
      miso_buf.insert(miso_buf.end(), pending_miso_buf.begin(),
                      pending_miso_buf.end());
      pending_miso_buf.clear();
    });
  }

  void
  SetMisoSizeHintCallback(std::function<void(std::size_t)> callback) final {
    miso_size_hint_callback = callback;
  }

  void ClearMisoSizeHintCallback() final { miso_size_hint_callback = {}; }

  void SetMosiCallback(
      std::function<void(std::span<const std::byte>)> callback) final {
    mosi_callback = callback;
  }

  void ClearMosiCallback() final { mosi_callback = {}; }

 private:
  [[nodiscard]] constexpr DurationUs
  TransmissionTime(std::size_t n_bytes) const {
    return DurationUs{static_cast<uint64_t>(std::ceil(
        static_cast<double>(8 * n_bytes) / static_cast<double>(f_clk)))};
  }

  Scheduler&  sched;
  std::string name;
  unsigned    f_clk;

  std::optional<std::function<void(std::size_t)>> miso_size_hint_callback{};
  std::optional<std::function<void(std::span<const std::byte>)>>
      mosi_callback{};

  ExternalEventItem&     miso_evt;
  std::vector<std::byte> pending_miso_buf{};
  std::deque<std::byte>  miso_buf{};
};

static_assert(hal::BlockingDuplexSpiMaster<BlockingSpiMaster<>>);

}   // namespace sil
