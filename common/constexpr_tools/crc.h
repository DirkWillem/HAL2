#pragma once

#include <cstdint>
#include <span>

namespace ct {

/**
 * Calculates the CRC16 of a given  amount of data
 * @param data Data to calculate CRC over
 * @param poly CRC polynomial
 * @return CRC
 */
[[nodiscard]] constexpr uint16_t Crc16(std::span<const std::byte> data,
                                       uint16_t poly = 0xA001) noexcept {
  uint16_t crc{0};
  for (auto byte : data) {
    crc ^= static_cast<uint16_t>(byte);
    for (auto i = 0; i < 8; i++) {
      if ((crc & 0b1U) != 0) {
        crc = (crc >> 1U) ^ poly;
      } else {
        crc >>= 1U;
      }
    }
  }

  return crc;
}

}   // namespace ct