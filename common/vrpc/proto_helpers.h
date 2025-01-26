#pragma once

#include <span>
#include <string_view>
#include <tuple>

#include <pb.h>
#include <pb_common.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include <halstd/mp/type_helpers.h>

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

[[nodiscard]] bool WriteProtoString(std::string_view src,
                                    std::span<char>  dst) noexcept;

template <typename Msg, typename F>
static std::optional<pb_field_iter_t>
FindFieldByPointer(const Msg&                        msg,
                   halstd::FieldPointer<Msg, F> auto field_ptr) {
  pb_field_iter_t iter{};
  pb_field_iter_begin_const(&iter, nanopb::MessageDescriptor<Msg>::fields(),
                            &msg);

  do {
    if (iter.pField == &(msg.*field_ptr)) {
      return {iter};
    }
  } while (pb_field_iter_next(&iter));

  return {};
}

template <typename Msg, typename El>
std::optional<std::span<const El>>
GetRepeatedFieldFromPtr(const Msg&                          msg,
                        halstd::FieldPointer<Msg, El*> auto field_ptr) {
  const auto iter_opt = FindFieldByPointer<Msg, El*>(msg, field_ptr);
  if (iter_opt.has_value()) {
    const auto iter = *iter_opt;
    if (iter.pSize == nullptr) {
      return {};
    }

    const auto count = static_cast<std::size_t>(
        *reinterpret_cast<const pb_size_t*>(iter.pSize));

    return std::span{reinterpret_cast<const El*>(iter.pData), count};
  }

  return {};
}

}   // namespace vrpc