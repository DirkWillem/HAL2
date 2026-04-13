module;

#include <array>
#include <cstring>
#include <exception>
#include <format>
#include <span>

export module hal.test.helpers:fakes.rtos_i2c;

import hstd;

import hal.abstract;

namespace hal::test::helpers {

/**
 * @brief Fake RTOS I2C. Currently only works with 8-bit addresses.
 */
export class FakeRtosI2c {
 public:
  // FAKED METHODS.
  I2cReadResult ReadMemory([[maybe_unused]] uint16_t device_address, I2cMemAddr auto memory_address,
                           std::span<std::byte> dest, [[maybe_unused]] hstd::Duration auto timeout,
                           std::optional<std::size_t> size = {}) {
    // Validate address type.
    if constexpr (!std::is_same_v<std::decay_t<decltype(memory_address)>, uint8_t>) {
      throw std::runtime_error{"Only 8-bit addresses are supported by FakeRtosI2c."};
    }

    // Validate address + read size.
    const auto rx_size = std::min(dest.size(), size.value_or(dest.size()));
    const auto addr    = static_cast<uint8_t>(memory_address);

    if (addr + rx_size > 0xFF) {
      throw std::runtime_error{std::format(
          "Reading {}-byte register at address {:x} will overflow address space.", rx_size, addr)};
    }

    // Perform "read".
    std::memcpy(dest.data(), &registers[addr], rx_size);

    return hal::I2cReadResult::Ok;
  }

  // TEST-ONLY METHODS.

  /**
   * @brief Sets a value in a register of the I2C device.
   * @tparam T Type of value to write.
   * @param address Address of the register to set. In case of multiple registers, the lowest
   * address.
   * @param value Value to write.
   */
  template <typename T>
  void SetRegister(uint8_t address, const T& value) {
    constexpr auto n    = sizeof(T);
    const auto     addr = static_cast<std::size_t>(address);

    // Validate address range.
    if (addr + n > 0xFF) {
      throw std::runtime_error{std::format(
          "Setting {}-byte register at address {:x} will overflow address space.", n, addr)};
    }

    // Set data.
    std::memcpy(&registers[addr], &value, sizeof(T));
  }

 private:
  std::array<std::byte, 0xFF> registers{};
};

}   // namespace hal::test::helpers