#pragma once

#include <algorithm>
#include <bit>
#include <cstring>
#include <span>
#include <utility>

#include "vrpc_uart_frames.h"

namespace vrpc::uart {

template <std::regular T>
[[nodiscard]] constexpr std::span<std::byte>
EncodeValue(T src, std::span<std::byte> dst, std::size_t& n_written) {
  if (sizeof(src) > dst.size()) {
    std::unreachable();
  }

  if consteval {
    using BackingArray = std::array<std::byte, sizeof(T)>;
    const auto value   = std::bit_cast<BackingArray>(src);
    std::copy(value.begin(), value.end(), dst.begin());
  } else {
    std::memcpy(dst.data(), &src, sizeof(src));
  }

  n_written += sizeof(src);

  return dst.subspan(sizeof(src));
}

constexpr std::span<const std::byte>
EncodeCommandFrame(std::span<std::byte> dst, uint32_t service_id,
                   uint32_t cmd_id, uint32_t req_id,
                   std::span<const std::byte> payload) noexcept {
  auto        cur_dst   = dst;
  std::size_t n_written = 2;

  cur_dst[0] = std::byte{FrameStart};
  cur_dst[1] = std::byte{FrameTypeCmdResponse};
  cur_dst    = cur_dst.subspan(2);

  cur_dst = EncodeValue(service_id, cur_dst, n_written);
  cur_dst = EncodeValue(cmd_id, cur_dst, n_written);
  cur_dst = EncodeValue(req_id, cur_dst, n_written);
  (void)EncodeValue(static_cast<uint32_t>(payload.size()), cur_dst, n_written);

  return dst.subspan(0, n_written + payload.size());
}

}   // namespace vrpc::uart