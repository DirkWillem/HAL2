module;

#include <cstring>
#include <span>
#include <string_view>
#include <tuple>

#include <tusb.h>

export module hal.usb;

import hstd;
import rtos.concepts;

namespace hal::usb {

inline constexpr uint16_t LangIdEnglish = 0x0409;

inline constexpr uint8_t StringIdLanguageId   = 0;
inline constexpr uint8_t StringIdManufacturer = 1;
inline constexpr uint8_t StringIdProduct      = 2;
inline constexpr uint8_t StringIdSerialNumber = 3;
inline constexpr uint8_t StringIdEndpoint0    = 4;

inline constexpr std::size_t DescriptorBufferSize = 32;

constexpr uint8_t InEndpointNum(uint8_t endpoint_num) {
  return 0x80 | endpoint_num;
}

constexpr uint8_t OutEndpointNum(uint8_t endpoint_num) {
  return endpoint_num;
}

export struct CdcAcmFunction {
  static constexpr std::size_t ConfigLen     = TUD_CDC_DESC_LEN;
  static constexpr std::size_t NumInterfaces = 2;

  uint8_t notification_endpoint;
  uint8_t data_endpoint;
  uint8_t notification_address = 16;
  uint8_t size                 = 64;

  constexpr std::array<uint8_t, ConfigLen>
  MakeConfig(uint8_t first_ifc_idx, uint8_t desc_idx) const noexcept {
    return {TUD_CDC_DESCRIPTOR(
        first_ifc_idx, static_cast<uint8_t>(StringIdEndpoint0 + desc_idx),
        InEndpointNum(notification_endpoint), notification_address,
        OutEndpointNum(data_endpoint), InEndpointNum(data_endpoint), size)};
  }
};

export void InitializeDevice() {
  tusb_rhport_init_t init = {
      .role  = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO,
  };
  tusb_rhport_init(BOARD_TUD_RHPORT, &init);
}

namespace concepts {

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

export template <typename T>
concept FunctionDescriptorTuple = IsFunctionDescriptorTuple<std::decay_t<T>>;

export template <typename DS>
concept DeviceDescription = requires {
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

}   // namespace concepts

/**
 * @brief Creates a USB device descriptor, to be used as the return value
 * of <c>tud_descriptor_device_cb</c>.
 *
 * @tparam D USB Device Description.
 * @return Buffer containing descriptor configuration.
 */
export template <concepts::DeviceDescription D>
consteval auto MakeDeviceDescriptor() {
  tusb_desc_device_t descriptor = {
      .bLength         = sizeof(tusb_desc_device_t),
      .bDescriptorType = TUSB_DESC_DEVICE,
      .bcdUSB          = 0x0100,

      .bDeviceClass    = TUSB_CLASS_MISC,
      .bDeviceSubClass = MISC_SUBCLASS_COMMON,
      .bDeviceProtocol = MISC_PROTOCOL_IAD,
      .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

      .idVendor  = D::VendorId,
      .idProduct = D::ProductId,
      .bcdDevice = D::DeviceBcd,

      .iManufacturer = StringIdManufacturer,
      .iProduct      = StringIdProduct,
      .iSerialNumber = StringIdSerialNumber,

      .bNumConfigurations = 1,
  };

  return std::bit_cast<std::array<uint8_t, sizeof(descriptor)>>(descriptor);
}

/**
 * @brief Creates a USB configuration descriptor, to be used as the return value
 * of <c>tud_descriptor_configuration_cb</c>.
 *
 * @tparam D USB Device Description.
 * @return Buffer containing descriptor configuration.
 */
export template <concepts::DeviceDescription D>
consteval auto MakeConfigurationDescriptor() {
  constexpr auto Functions    = D::Functions;
  using FunctionsTuple        = std::decay_t<decltype(Functions)>;
  constexpr auto NumFunctions = std::tuple_size_v<FunctionsTuple>;

  // Calculate result size.
  constexpr std::size_t FunctionsSize =
      []<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
        return (... + std::tuple_element_t<Idxs, FunctionsTuple>::ConfigLen);
      }(std::make_index_sequence<NumFunctions>());

  constexpr std::size_t ResultSize = TUD_CONFIG_DESC_LEN + FunctionsSize;
  std::array<uint8_t, ResultSize> result{};

  // Add header.
  constexpr std::size_t NumInterfaces = []<std::size_t... Idxs>(
                                            std::index_sequence<Idxs...>) {
    return (... + std::tuple_element_t<Idxs, FunctionsTuple>::NumInterfaces);
  }(std::make_index_sequence<NumFunctions>());

  std::array<uint8_t, TUD_CONFIG_DESC_LEN> header{TUD_CONFIG_DESCRIPTOR(
      1, NumInterfaces, 0, static_cast<uint8_t>(ResultSize), 0x00, 100)};
  std::copy(header.begin(), header.end(), result.begin());

  // Add functions.
  [&result, &Functions]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
    uint8_t     first_interface_idx = 0;
    uint8_t     function_idx        = 0;
    std::size_t offset              = TUD_CONFIG_DESC_LEN;

    (
        [&result, &first_interface_idx, &function_idx, &offset,
         &Functions]<std::size_t Idx>(hstd::ValueMarker<Idx>) {
          using F = std::tuple_element_t<Idx, FunctionsTuple>;

          // Create buffer for current function and add to result buffer.
          const auto data = std::get<Idx>(Functions).MakeConfig(
              first_interface_idx, function_idx);
          std::copy(data.begin(), data.end(), &result[offset]);

          // Increment counters and offsets.
          offset += data.size();
          first_interface_idx += F::NumInterfaces;
          function_idx += 1;
        }(hstd::ValueMarker<Idxs>()),
        ...);
  }(std::make_index_sequence<NumFunctions>());

  return result;
}

export template <rtos::concepts::Rtos OS>
class UsbTask : public OS::template Task<UsbTask<OS>, OS::MediumStackSize> {
 public:
  template <typename... Args>
  explicit UsbTask(Args&&... base_args)
      : OS::template Task<UsbTask, OS::MediumStackSize>{
            std::forward<Args>(base_args)...} {}

  [[noreturn]] void operator()() {
    while (true) {
      tud_task();
    }
  }
};

uint16_t HeaderByte(uint16_t length) {
  return ((TUSB_DESC_STRING << 8) | (2 * length + 2));
}

void WriteDescriptor(std::span<uint16_t> buffer, uint16_t value) {
  buffer[0] = HeaderByte(1);
  buffer[1] = value;
}

void WriteDescriptor(std::span<uint16_t> buffer, std::string_view value) {
  const auto len = std::min(value.length(), DescriptorBufferSize);

  buffer[0] = HeaderByte(len);

  for (std::size_t i = 0; i < len; ++i) {
    buffer[i + 1] = static_cast<uint16_t>(value[i]);
  }
}

export template <concepts::DeviceDescription DS>
const uint16_t* GetDescriptorString(uint8_t index, uint16_t) {
  static std::array<uint16_t, DescriptorBufferSize + 1> buffer{};

  switch (index) {
  case StringIdLanguageId: WriteDescriptor(buffer, LangIdEnglish); break;
  case StringIdManufacturer: WriteDescriptor(buffer, DS::Manufacturer); break;
  case StringIdProduct: WriteDescriptor(buffer, DS::Product); break;
  case StringIdSerialNumber: WriteDescriptor(buffer, DS::SerialNumber); break;
  default: {
    auto       endpoint_idx     = index - StringIdEndpoint0;
    const auto endpoint_strings = std::span{DS::Endpoints};
    if (endpoint_idx < endpoint_strings.size()) {
      WriteDescriptor(buffer, endpoint_strings[endpoint_idx]);
    } else {
      return nullptr;
    }
  }
  }

  return buffer.data();
}

}   // namespace hal::usb
