#pragma once

#include <bit>
#include <functional>
#include <memory>
#include <span>

#include <pb.h>
#include <pb_encode.h>

#include "proto_helpers.h"

namespace helpers {

class BufferBuilder {
 public:
  explicit BufferBuilder(std::span<std::byte> buffer);

  template <typename T>
  auto& Write(T value) {
    if (buffer.size() < sizeof(T)) {
      throw std::runtime_error{"Value is too large to write to buffer"};
    }

    std::memcpy(buffer.data(), reinterpret_cast<void*>(&value), sizeof(T));
    buffer = buffer.subspan(sizeof(T));
    return *this;
  }

  template <typename T>
  auto& WriteProto(const T& message) {
    WriteProto(message, []() {});
  }

  template <typename Msg>
  auto& WriteProto(const Msg&                              message,
                   const std::function<void(std::size_t)>& pre_write_callback) {
    using Descriptor = nanopb::MessageDescriptor<Msg>;
    std::array<std::byte, Descriptor::size> msg_buf{};

    const auto written = helpers::EncodeProto(
        message, std::span<std::byte>{msg_buf}, pre_write_callback);

    if (buffer.size() < written) {
      throw std::runtime_error{"Proto message is too large to write to buffer"};
    }

    std::memcpy(buffer.data(), reinterpret_cast<void*>(&msg_buf), written);
    buffer = buffer.subspan(written);
    return *this;
  }

  BufferBuilder& WriteCrc16(uint16_t poly, std::size_t offset = 0) noexcept;

  [[nodiscard]] std::size_t          BytesWritten() const noexcept;
  [[nodiscard]] std::span<std::byte> Bytes() const noexcept;

 private:
  std::span<std::byte> original_buffer{};
  std::span<std::byte> buffer{};
};

}   // namespace helpers