module;

#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <tusb.h>

export module hal.usb:functions.cdc;

import :concepts;
import :helpers;

namespace hal::usb {

/**
 * @brief Function descriptor for a CDC-ACM function. Conforms to the
 * <c>hal::usb::FunctionDescriptor</c> concept.
 */
export struct CdcAcmFunction {
  static constexpr std::size_t ConfigLen     = TUD_CDC_DESC_LEN;
  static constexpr std::size_t NumInterfaces = 2;

  uint8_t notification_endpoint;   //!< Notification endpoint number.
  uint8_t data_endpoint;           //!< Data endpoint number.
  uint8_t notification_address = 16;
  uint8_t size                 = 64;

  /**
   * @brief Creates a function descriptor for this CDC-ACM function.
   *
   * @param first_ifc_idx Index of the first interface of this function.
   * @param desc_idx Index of this descriptor.
   * @return Function descriptor.
   */
  constexpr std::array<uint8_t, ConfigLen>
  MakeConfig(uint8_t first_ifc_idx, uint8_t desc_idx) const noexcept {
    return {TUD_CDC_DESCRIPTOR(
        first_ifc_idx, static_cast<uint8_t>(StringIdEndpoint0 + desc_idx),
        InEndpointNum(notification_endpoint), notification_address,
        OutEndpointNum(data_endpoint), InEndpointNum(data_endpoint), size)};
  }

  /**
   * @brief Equality comparison operator.
   *
   * @param rhs \c CdcAcmFunction to compare to.
   * @return Whether \rhs is equal to this <c>CdcAcmFunction</c>.
   */
  constexpr bool operator==(const CdcAcmFunction& rhs) const noexcept {
    return notification_endpoint == rhs.notification_endpoint
           && data_endpoint == rhs.data_endpoint
           && notification_address == rhs.notification_address
           && size == rhs.size;
  }
};

/**
 * @brief Single CDC-ACM function within the USB device.
 */
export class CdcAcmInterface {
  /**
   * @brief Finds the index of the given CDC-ACM function.
   *
   * @tparam D Device description.
   * @param func Function to find.
   * @return CDC interface index of the CDC-ACM function.
   */
  template <concepts::DeviceDescription D>
  static consteval uint8_t GetIndex(const CdcAcmFunction& func) {
    using FunctionsTuple          = std::decay_t<decltype(D::Functions)>;
    std::optional<uint8_t> result = std::nullopt;

    // Loop over functions to find the requested function. To do this, do some
    // lambda and pack expansion trickery.
    [&func, &result]<std::size_t... Is>(std::index_sequence<Is...>) {
      uint8_t current_idx = 0;

      (...,
       [&func, &result, &current_idx]<std::size_t I>(hstd::ValueMarker<I>) {
         using F = std::decay_t<std::tuple_element_t<I, FunctionsTuple>>;
         if constexpr (std::is_same_v<F, CdcAcmFunction>) {
           const F& f = std::get<I>(D::Functions);
           if (f == func) {
             result = current_idx;
           } else {
             ++current_idx;
           }
         }
       }(hstd::ValueMarker<Is>()));
    }(std::make_index_sequence<std::tuple_size_v<FunctionsTuple>>());

    // Invoke UB if the function was not found. Because this function is
    // consteval, this will result in a compile-time error.
    if (!result.has_value()) {
      std::unreachable();
    }

    return *result;
  }

 public:
  /**
   * @brief Obtains a CDC-ACM interface instance.
   *
   * @tparam D USB device description.
   * @tparam F CDC-ACM function to get the CDC-ACM interface for.
   * @return \c CdcAcmInterface instance.
   */
  template <concepts::DeviceDescription D, CdcAcmFunction F>
  static CdcAcmInterface Get() {
    constexpr auto Idx = GetIndex<D>(F);
    return CdcAcmInterface{Idx};
  }

  /**
   * @brief Returns the number of bytes that are available to be read.
   *
   * @return Number of bytes available to read.
   */
  std::size_t BytesAvailable() const noexcept {
    return tud_cdc_n_available(interface);
  }

  /**
   * @brief Reads bytes from the buffer.
   *
   * @param buffer Buffer to read into.
   * @return View over the read bytes, or <c>std::nullopt</c> if no bytes were
   * read.
   */
  std::optional<std::span<const std::byte>>
  Read(std::span<std::byte> buffer) noexcept {
    const auto n =
        tud_cdc_n_read(interface, buffer.data(), buffer.size_bytes());

    if (n == 0) {
      return std::nullopt;
    }

    return buffer.subspan(0, n);
  }

  /**
   * @brief Writes data to the CDC-ACM interface, optionally flusing the buffer
   * .
   * @param data Data to write.
   * @param flush Whether to flush after writing.
   */
  void Write(std::span<const std::byte> data, bool flush = true) noexcept {
    tud_cdc_n_write(interface, data.data(), data.size_bytes());

    if (flush) {
      FlushWrite();
    }
  }

  /**
   * @brief Writes a string to the CDC-ACM interface, optionally flusing the
   * buffer.
   *
   * @param sv String to write.
   * @param flush Whether to flush after writing.
   */
  void Write(std::string_view sv, bool flush = true) noexcept {
    tud_cdc_n_write(interface, sv.data(), sv.size());

    if (flush) {
      FlushWrite();
    }
  }

  /**
   * @brief Flushes the write buffer.
   */
  void FlushWrite() noexcept { tud_cdc_n_write_flush(interface); }

 private:
  explicit CdcAcmInterface(uint8_t interface)
      : interface{interface} {}

  uint8_t interface;   //!< Interface number.
};

}   // namespace hal::usb