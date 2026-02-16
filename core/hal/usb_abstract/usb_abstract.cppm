module;

#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

export module hal.usb.abstract;

namespace hal::usb::concepts {

export template <typename Impl>
concept CdcAcmInterface = requires(Impl& ifc, const Impl& c_ifc) {
  // Read API
  { c_ifc.BytesAvailable() } -> std::convertible_to<std::size_t>;
  {
    ifc.Read(std::declval<std::span<std::byte>>())
  } -> std::convertible_to<std::optional<std::span<const std::byte>>>;

  // Write API
  ifc.Write(std::declval<std::string_view>());
  ifc.Write(std::declval<std::string_view>(), std::declval<bool>());
  ifc.Write(std::declval<std::span<const std::byte>>());
  ifc.Write(std::declval<std::span<const std::byte>>(), std::declval<bool>());
  ifc.FlushWrite();
};

}   // namespace hal::usb::concepts