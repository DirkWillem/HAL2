module;

#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>

export module vrpc.common:proto_helpers;

import hstd;
import nanopb;

namespace vrpc {

/**
 * Decodes a protobuf message from the given buffer into the given message
 * @tparam Msg Message type to decode
 * @param src Source buffer
 * @param dst Destination message
 * @return Whether decoding was successful
 */
export template <typename Msg>
[[nodiscard]] bool ProtoDecode(std::span<const std::byte> src,
                               Msg&                       dst) noexcept {
  auto istream = pb_istream_from_buffer(
      hstd::ReinterpretSpan<pb_byte_t>(src).data(), src.size());
  return pb_decode(&istream, nanopb::MsgDescriptor<Msg>::fields(), &dst);
}

/**
 * Encodes a protobuf message into the given buffer
 * @tparam Msg Message type
 * @param src Source message
 * @param dst Destination buffer
 * @return Whether encoding was successful
 */
export template <typename Msg>
[[nodiscard]] std::tuple<bool, std::span<const std::byte>>
ProtoEncode(const Msg& src, std::span<std::byte> dst) noexcept {
  auto ostream = pb_ostream_from_buffer(
      hstd::ReinterpretSpanMut<pb_byte_t>(dst).data(), dst.size());
  if (!pb_encode(&ostream, nanopb::MsgDescriptor<Msg>::fields(), &src)) {
    return {false, {}};
  }

  return {true, dst.subspan(0, ostream.bytes_written)};
}

/**
 * Writes a string to a given buffer, omitting characters if the buffer size is
 * not sufficient
 * @param src String to write
 * @param dst Destination buffer
 * @return Whether the full string could be written
 */
export [[nodiscard]] bool WriteProtoString(std::string_view src,
                                           std::span<char>  dst) noexcept {
  const auto n_chars_to_write = std::min(src.length(), dst.size() - 1);

  std::memcpy(dst.data(), src.data(), n_chars_to_write);
  dst[n_chars_to_write] = '\0';

  return n_chars_to_write == src.size();
}

/**
 * Finds a protobuf field by its field pointer
 * @tparam Msg Message type
 * @tparam F Field pointer type
 * @param msg Message instance
 * @param field_ptr Field pointer
 * @return Field iterator
 */
export template <typename Msg, typename F>
std::optional<pb_field_iter_t>
FindFieldByPointer(const Msg& msg, hstd::FieldPointer<Msg, F> auto field_ptr) {
  pb_field_iter_t iter{};
  pb_field_iter_begin_const(&iter, nanopb::MsgDescriptor<Msg>::fields(),
                            &msg);

  // NOLINTBEGIN(cppcoreguidelines-avoid-do-while)
  do {
    if (iter.pField == &(msg.*field_ptr)) {
      return {iter};
    }
  } while (pb_field_iter_next(&iter));
  // NOLINTEND(cppcoreguidelines-avoid-do-while)

  return {};
}

/**
 * Returns a view over a repeated message field given its field pointer
 * @tparam Msg Message type
 * @tparam El Element type
 * @param msg Message to obtain field from
 * @param field_ptr Field pointer to the repeated type
 * @return Span over the repeated field
 */
export template <typename Msg, typename El>
std::optional<std::span<const El>>
GetRepeatedFieldFromPtr(const Msg&                        msg,
                        hstd::FieldPointer<Msg, El*> auto field_ptr) {
  const auto iter_opt = FindFieldByPointer<Msg, El*>(msg, field_ptr);
  if (iter_opt.has_value()) {
    const auto iter = *iter_opt;
    if (iter.pSize == nullptr) {
      return {};
    }

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto count = static_cast<std::size_t>(
        *reinterpret_cast<const pb_size_t*>(iter.pSize));

    return std::span{reinterpret_cast<const El*>(iter.pData), count};
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
  }

  return {};
}

}   // namespace vrpc
