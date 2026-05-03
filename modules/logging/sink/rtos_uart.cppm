module;

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <span>

export module logging.sink.rtos_uart;

import hal.abstract;

import rtos.concepts;

/**
 * @brief Logging sink that logs to a UART, and polls the
 */
namespace logging::sink {
export template <rtos::concepts::Rtos OS, hal::RtosUart Uart>
class RtosUart : public OS::template Task<RtosUart<OS, Uart>, OS::MediumStackSize> {
  static constexpr std::size_t BufSize       = 256;
  static constexpr uint32_t    Buf1TxDoneBit = 0b01U;
  static constexpr uint32_t    Buf2TxDoneBit = 0b10U;

 public:
  explicit RtosUart(Uart& uart)
      : OS::template Task<RtosUart, OS::MediumStackSize>{"RtosUartSink"}
      , uart{uart} {
    // Mark both buffers as transmitted
    event_group.SetBits(Buf1TxDoneBit | Buf2TxDoneBit);
  }

  void operator()() {
    using namespace std::chrono_literals;

    while (!this->StopRequested()) {
      OS::System::Clock::BlockFor(50ms);
      TransmitWriteBuffer();
    }
  }

  /**
   * @brief Writes data to the sink.
   * @param data Data to write to the sink.
   */
  void Write(std::span<const std::byte> data) noexcept {
    // If the message is larger than the buffer size, we can never transmit.
    if (data.size() > BufSize) {
      return;
    }

    // See if we need to transmit the current buffer.
    if (data.size() > free_write_buf.size()) {
      if (!TransmitWriteBuffer()) {
        return;
      }
    }

    // Write the message to the current write buffer.
    std::memcpy(free_write_buf.data(), data.data(), data.size());
    free_write_buf = free_write_buf.subspan(data.size());
  }

 private:
  /**
   * @brief Transmits the data present in the write buffer.
   * @return Whether transmitting the write buffer was successful.
   */
  bool TransmitWriteBuffer() {
    using namespace std::chrono_literals;

    const auto size_to_transmit = BufSize - free_write_buf.size();

    // Determine the TX done bit for the current buffer.
    const auto tx_done_bit = write_buf_idx == 0 ? Buf1TxDoneBit : Buf2TxDoneBit;

    // Wait for the buffer to finish transmitting.
    if (!event_group.Wait(tx_done_bit, 10ms, true, false)) {
      return false;
    }

    // Transmit the buffer and swap the buffer
    write_buf_idx  = (write_buf_idx + 1) % 2;
    free_write_buf = buffers[write_buf_idx];
    return true;
  }

  Uart&          uart;
  OS::EventGroup event_group{};

  std::array<std::array<std::byte, BufSize>, 2> buffers{};
  std::size_t                                   write_buf_idx{0};
  std::span<std::byte>                          free_write_buf{buffers[write_buf_idx]};
};

}   // namespace logging::sink