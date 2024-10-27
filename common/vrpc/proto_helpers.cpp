#include "proto_helpers.h"

#include <cstring>

namespace vrpc {

bool WriteProtoString(std::string_view src, std::span<char> dst) noexcept {
  const auto n_chars_to_write = std::min(src.length(), dst.size() - 1);

  std::memcpy(dst.data(), src.data(), n_chars_to_write);
  dst[n_chars_to_write] = '\0';

  return n_chars_to_write == src.size();
}

}   // namespace vrpc