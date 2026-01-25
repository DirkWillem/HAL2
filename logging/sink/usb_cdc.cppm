module;

#include <bit>
#include <span>

export module logging:sink.usb_cdc;

import hal.usb.abstract;

namespace logging::sink {

/**
 * @brief Logging sink that logs to a USB CDC-ACM interface.
 * @tparam Ifc USB CDC-ACM interface to use for logging.
 */
export template <hal::usb::concepts::CdcAcmInterface Ifc>
class UsbCdc {
 public:
  /**
   * @brief Constructor.
   * @param interface CDC-ACM interface to use.
   */
  UsbCdc(Ifc& interface)
      : interface{interface} {}

  /**
   * @brief Writes an encoded message.
   * @param data Data to write.
   */
  void Write(std::span<const std::byte> data) noexcept {
    interface.Write(data, true);
  }

 private:
  Ifc& interface;
};

}   // namespace logging::sink
