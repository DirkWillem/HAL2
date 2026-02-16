module;

#include <cstdint>

export module hal.usb:helpers;

namespace hal::usb {

export inline constexpr uint16_t LangIdEnglish = 0x0409;

export inline constexpr uint8_t StringIdLanguageId   = 0;
export inline constexpr uint8_t StringIdManufacturer = 1;
export inline constexpr uint8_t StringIdProduct      = 2;
export inline constexpr uint8_t StringIdSerialNumber = 3;
export inline constexpr uint8_t StringIdEndpoint0    = 4;

export inline constexpr std::size_t DescriptorBufferSize = 32;

constexpr uint8_t InEndpointNum(uint8_t endpoint_num) {
  return 0x80 | endpoint_num;
}

constexpr uint8_t OutEndpointNum(uint8_t endpoint_num) {
  return endpoint_num;
}

}   // namespace hal::usb
