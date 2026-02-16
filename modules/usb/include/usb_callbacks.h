#pragma once

extern "C" {

using DD =
    hal::usb::UsbDeviceDescriptor<hal::usb::UsbDeviceDescriptorImplMarker>;

static constexpr auto DeviceDescriptor = hal::usb::MakeDeviceDescriptor<DD>();

const uint8_t* tud_descriptor_device_cb() {
  return DeviceDescriptor.data();
}

static constexpr auto Descriptor = hal::usb::MakeConfigurationDescriptor<DD>();

uint8_t const* tud_descriptor_configuration_cb(uint8_t) {
  return Descriptor.data();
}

static_assert(hal::usb::concepts::DeviceDescription<DD>);

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  return hal::usb::GetDescriptorString<DD>(index, langid);
}
}
