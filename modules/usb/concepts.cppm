module;

#include <concepts>
#include <cstdint>
#include <string_view>
#include <tuple>

export module hal.usb:concepts;

import hstd;

namespace hal::usb::concepts {

/**
 * @brief Concept describing a USB function descriptor.
 */
export template <typename FD>
concept FunctionDescriptor = requires(const FD& cfd) {
  { std::decay_t<FD>::ConfigLen } -> std::convertible_to<std::size_t>;
  { std::decay_t<FD>::NumInterfaces } -> std::convertible_to<uint8_t>;

  {
    cfd.MakeConfig(std::declval<uint8_t>(), std::declval<uint8_t>())
  } -> std::convertible_to<std::array<uint8_t, std::decay_t<FD>::ConfigLen>>;
};

template <typename T>
inline constexpr bool IsFunctionDescriptorTuple = false;

template <typename... Ts>
inline constexpr bool IsFunctionDescriptorTuple<std::tuple<Ts...>> =
    (... && FunctionDescriptor<Ts>);

/**
 * @brief Concept describing a <c>std::tuple</c> of \c FunctionDescriptor types.
 */
export template <typename T>
concept FunctionDescriptorTuple = IsFunctionDescriptorTuple<std::decay_t<T>>;

/**
 * @brief Concept describing a USB device descriptor.
 */
export template <typename DS>
concept DeviceDescription = requires {
  // ID's and other numeric values.
  { DS::VendorId } -> std::convertible_to<uint16_t>;
  { DS::ProductId } -> std::convertible_to<uint16_t>;
  { DS::DeviceBcd } -> std::convertible_to<uint16_t>;

  { DS::Manufacturer } -> std::convertible_to<std::string_view>;
  { DS::Product } -> std::convertible_to<std::string_view>;
  { DS::SerialNumber } -> std::convertible_to<std::string_view>;

  // DS::Functions must be a tuple of values that are a FunctionDescriptor.
  { DS::Functions } -> FunctionDescriptorTuple;

  // DS::Endpoints must be a std::array<std::string_view, N>.
  requires hstd::IsArray<std::decay_t<decltype(DS::Endpoints)>>;
  requires std::convertible_to<
      typename std::decay_t<decltype(DS::Endpoints)>::value_type,
      std::string_view>;
};

}   // namespace hal::usb::concepts