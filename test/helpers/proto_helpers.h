#pragma once

#include <functional>
#include <span>
#include <sstream>

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

namespace helpers {

template <typename Msg, typename B>
  requires(sizeof(B) == 1)
std::size_t EncodeProto(const Msg& msg, std::span<B> into,
                        const std::function<void(std::size_t)>& size_callback) {
  // Encode into temporary buffer
  using Descriptor = nanopb::MessageDescriptor<Msg>;
  std::array<uint8_t, Descriptor::size> msg_buf{};
  auto ostream = pb_ostream_from_buffer(msg_buf.data(), msg_buf.size());
  if (!pb_encode(&ostream, Descriptor::fields(), &msg)) {
    std::stringstream ss{};
    ss << "Failed to encode: " << ostream.errmsg;
    throw std::runtime_error{ss.str()};
  }

  // Call size callback
  size_callback(ostream.bytes_written);

  // Check buffer size and copy data to buffer
  if (into.size() < ostream.bytes_written) {
    throw std::runtime_error{"Proto message is too large to write to buffer"};
  }

  std::memcpy(into.data(), msg_buf.data(), ostream.bytes_written);
  return ostream.bytes_written;
}

template <typename Msg, typename B>
  requires(sizeof(B) == 1)
std::size_t EncodeProto(const Msg& msg, std::span<B> into) {
  return Encode(msg, into, [](auto) {});
}

template <typename Msg, typename B>
  requires(sizeof(B) == 1)
void DecodeProtoInto(Msg& into, std::span<B> buffer) {
  using Descriptor = nanopb::MessageDescriptor<Msg>;
  auto istream     = pb_istream_from_buffer(
      reinterpret_cast<const pb_byte_t*>(buffer.data()), buffer.size());

  if (!pb_decode(&istream, Descriptor::fields(), &into)) {
    std::stringstream ss{};
    ss << "Failed to encode: " << istream.errmsg;
    throw std::runtime_error{ss.str()};
  }
}

template <typename Msg, typename B>
  requires(sizeof(B) == 1)
Msg DecodeProto(std::span<B> buffer) {
  Msg result{};
  DecodeProtoInto(result, buffer);
  return result;
}

}   // namespace helpers
