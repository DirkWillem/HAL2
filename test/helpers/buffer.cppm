module;

#include <bit>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <span>

export module testing.helpers:buffers;

import hstd;

namespace helpers {

export struct BufferBuilderSettings {
  uint16_t default_crc16_poly = 0xA001;
  uint16_t default_crc16_init = 0x0000;
};

export template <std::endian E, std::endian CE = E>
class BufferBuilder {
 public:
  explicit BufferBuilder(std::span<std::byte>  buffer,
                         BufferBuilderSettings settings = {}) noexcept
      : original_buffer{buffer}
      , buffer{buffer}
      , settings{settings} {}

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

  [[nodiscard]] std::size_t BytesWritten() const noexcept {
    return buffer.data() - original_buffer.data();
  }
  [[nodiscard]] std::span<std::byte> Bytes() const noexcept {
    return original_buffer.subspan(0, BytesWritten());
  }

 private:
  std::span<std::byte> original_buffer{};
  std::span<std::byte> buffer{};

  BufferBuilderSettings settings;
};

}   // namespace helpers