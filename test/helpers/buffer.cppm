module;

#include <bit>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <span>

export module hal2.testing.helpers:buffers;

import hstd;

namespace hal2::testing::helpers {

/** Buffer builder settings */
export struct BufferBuilderSettings {
  uint16_t default_crc16_poly = 0xA001;   //!< Default CRC-16 polynomial
  uint16_t default_crc16_init = 0x0000;   //!< Default CRC-16 initial value
};

/**
 * @brief Helper class for building byte buffers in tests.
 *
 * @tparam E Buffer endianness.
 * @tparam CE CRC endianness.
 */
export template <std::endian E, std::endian CE = E>
class BufferBuilder {
 public:
  /**
   * @brief Constructor.
   *
   * @param buffer View over the underlying buffer to write to.
   * @param settings Buffer builder settings.
   */
  explicit BufferBuilder(std::span<std::byte>  buffer,
                         BufferBuilderSettings settings = {}) noexcept
      : original_buffer{buffer}
      , buffer{buffer}
      , settings{settings} {}

  /**
   * @brief Writes an unsigned integer value to the buffer.
   *
   * @tparam T Unsigned integer type.
   * @tparam VE Endianness to write the value with.
   * @param value Value to write.
   * @return Reference to the buffer builder.
   */
  template <std::unsigned_integral T, std::endian VE = E>
  auto& Write(T value) {
    if (buffer.size() < sizeof(T)) {
      throw std::runtime_error{"Value is too large to write to buffer"};
    }

    const auto tmp = hstd::ConvertToEndianness<VE>(value);

    std::memcpy(buffer.data(), &tmp, sizeof(T));
    buffer = buffer.subspan(sizeof(T));
    return *this;
  }

  /**
   * @brief Writes a byte to the buffer.
   *
   * @param value Value to write
   * @return Reference to the buffer builder.
   */
  auto& Write(std::byte value) {
    if (buffer.size() < sizeof(value)) {
      throw std::runtime_error{"Value is too large to write to buffer"};
    }

    buffer[0] = value;
    buffer    = buffer.subspan(sizeof(std::byte));
    return *this;
  }

  /**
   * @brief Fills the buffer with a fixed number of bytes, all having the same
   * value.
   *
   * @param value Byte to fill with.
   * @param count Number of bytes to fill with.
   * @return Reference to the buffer builder.
   */
  auto& FillBytes(std::byte value, std::size_t count) {
    if (buffer.size() < sizeof(count)) {
      throw std::runtime_error{"Value is too large to write to buffer"};
    }

    for (std::size_t i = 0; i < count; ++i) {
      buffer[i] = value;
    }
    buffer = buffer.subspan(count);
    return *this;
  }

  /**
   * @brief Writes a floating-point value to the buffer.
   *
   * @tparam VE Endianness to write the value with.
   * @param value Value to write.
   * @return Reference to the buffer builder.
   */
  template <std::endian VE = E>
  auto& Write(float value) {
    if (buffer.size() < sizeof(float)) {
      throw std::runtime_error{"Value is too large to write to buffer"};
    }

    const auto tmp = hstd::ConvertToEndianness<VE>(value);

    std::memcpy(buffer.data(), &tmp, sizeof(float));
    buffer = buffer.subspan(sizeof(float));
    return *this;
  }

  /**
   * @brief Writes a CRC-16 checksum of the data written to the buffer to the
   * buffer.
   *
   * @param poly CRC-16 polynomial.
   * @param init CRC-16 iniital value.
   * @param offset Offset into the buffer to start calculating the CRC.
   * @return Reference to the buffer builder.
   */
  BufferBuilder& WriteCrc16(std::optional<uint16_t> poly   = {},
                            std::optional<uint16_t> init   = {},
                            std::size_t             offset = 0) noexcept {
    const auto crc =
        hstd::Crc16(original_buffer.subspan(offset, BytesWritten() - offset),
                    poly.value_or(settings.default_crc16_poly),
                    init.value_or(settings.default_crc16_init));
    Write<uint16_t, CE>(crc);
    return *this;
  }

  /**
   * @brief Writes an invalid CRC-16 checksum of the data written to the buffer
   * to the buffer.
   *
   * @note The invalid checksum is calculated as the bitwise NOT of the correct
   * checksum.
   *
   * @param poly CRC-16 polynomial.
   * @param init CRC-16 iniital value.
   * @param offset Offset into the buffer to start calculating the CRC.
   * @return Reference to the buffer builder.
   */
  BufferBuilder& WriteInvalidCrc16(std::optional<uint16_t> poly = {},
                                   std::optional<uint16_t> init = {},
                                   std::size_t offset           = 0) noexcept {
    const auto crc =
        hstd::Crc16(original_buffer.subspan(offset, BytesWritten() - offset),
                    poly.value_or(settings.default_crc16_poly),
                    init.value_or(settings.default_crc16_init));
    Write<uint16_t, CE>(~crc);
    return *this;
  }

  /**
   * @brief Returns the number of bytes written to the buffer.
   *
   * @return Number of bytes written to the buffer.
   */
  [[nodiscard]] std::size_t BytesWritten() const noexcept {
    return buffer.data() - original_buffer.data();
  }

  /**
   * @brief Returns a read-only view over the data written to the buffer.
   *
   * @return View over the written data.
   */
  [[nodiscard]] std::span<const std::byte> Bytes() const noexcept {
    return original_buffer.subspan(0, BytesWritten());
  }

 private:
  std::span<std::byte> original_buffer{};   //!< View over the full buffer
  std::span<std::byte> buffer{};   //!< View over the part of the buffer that
                                   //!< hasn't been written to yet

  BufferBuilderSettings settings;   //!< Buffer builder settings
};

}   // namespace hal2::helpers