#pragma once

#include <span>
#include <string_view>
#include <tuple>

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

namespace vrpc {

template <typename Msg>
[[nodiscard]] bool ProtoDecode(std::span<const std::byte> src,
                               Msg&                       dst) noexcept {
  auto istream = pb_istream_from_buffer(
      reinterpret_cast<const pb_byte_t*>(src.data()), src.size());
  return pb_decode(&istream, nanopb::MessageDescriptor<Msg>::fields(), &dst);
}

template <typename Msg>
[[nodiscard]] std::tuple<bool, std::span<const std::byte>>
ProtoEncode(const Msg& src, std::span<std::byte> dst) noexcept {
  auto ostream = pb_ostream_from_buffer(
      reinterpret_cast<pb_byte_t*>(dst.data()), dst.size());
  if (!pb_encode(&ostream, nanopb::MessageDescriptor<Msg>::fields(), &src)) {
    return {false, {}};
  }

  return {true, dst.subspan(0, ostream.bytes_written)};
}

[[nodiscard]] bool WriteProtoString(std::string_view src, std::span<char> dst) noexcept;

}   // namespace vrpc