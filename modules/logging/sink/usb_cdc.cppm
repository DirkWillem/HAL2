module;

#include <bit>
#include <chrono>
#include <cstring>
#include <span>

export module logging.sink.usb;

import hal.usb.abstract;

import rtos.concepts;

namespace logging::sink {

/**
 * @brief Logging sink that logs to a USB CDC-ACM interface.
 * @tparam OS RTOS type.
 * @tparam Ifc USB CDC-ACM interface to use for logging.
 */
export template <rtos::concepts::Rtos OS, hal::usb::concepts::CdcAcmInterface Ifc>
class UsbCdc {
 public:
  /**
   * @brief Constructor.
   * @param interface CDC-ACM interface to use.
   */
  explicit UsbCdc(Ifc& interface)
      : interface{interface} {}

  /**
   * @brief Writes an encoded message.
   * @param data Data to write.
   */
  void Write(std::span<const std::byte> data) noexcept {
    using namespace std::chrono_literals;

    // Message can directly be written.
    if (interface.ReadyToWrite()) {
      interface.Write(data, true);
    }
  }

 private:
  Ifc& interface;
};

}   // namespace logging::sink
