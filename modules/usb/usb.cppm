module;

#include <chrono>
#include <cstring>
#include <span>
#include <string_view>
#include <tuple>

#include <tusb.h>

export module hal.usb;

import hstd;
import rtos.concepts;

export import :concepts;
import :helpers;

export import :functions.cdc;

namespace hal::usb {

export void InitializeDevice() {
  tusb_rhport_init_t init = {
      .role  = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO,
  };
  tusb_rhport_init(BOARD_TUD_RHPORT, &init);
}

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
            std::forward<Args>(base_args)..., configMAX_PRIORITIES - 2} {}

  [[noreturn]] void operator()() {
    using namespace std::chrono_literals;
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

export struct UsbDeviceDescriptorImplMarker {};

export template <typename IM>
struct UsbDeviceDescriptor;

}   // namespace hal::usb
