#include "buffer_builder.h"

#include <constexpr_tools/crc.h>

namespace helpers {

BufferBuilder::BufferBuilder(std::span<std::byte> buffer)
    : original_buffer{buffer}
    , buffer{buffer} {}

BufferBuilder& BufferBuilder::WriteCrc16(uint16_t    poly,
                                         std::size_t offset) noexcept {
  const auto crc =
      ct::Crc16(original_buffer.subspan(offset, BytesWritten() - offset), poly);
  Write(crc);
  return *this;
}

std::size_t BufferBuilder::BytesWritten() const noexcept {
  return buffer.data() - original_buffer.data();
}

std::span<std::byte> BufferBuilder::Bytes() const noexcept {
  return original_buffer.subspan(0, BytesWritten());
}

}   // namespace helpers