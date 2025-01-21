#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace vrpc {

struct FrameFormat {
  bool has_server_addr_word = false;
};

/**
 * Command request frame
 *
 * SOF = 1 byte ('V')
 * CMD frame = 1 byte ('C')
 * Service ID = 4 bytes
 * Command ID = 4 bytes
 * Request ID = 4 bytes
 * Payload Length = 4 bytes
 * Payload = N bytes
 * CRC = 2 bytes
 */

static constexpr uint16_t CrcPoly = 0xA001;

inline constexpr auto FrameStart                  = std::byte{'V'};
inline constexpr auto FrameTypeCmdRequest         = std::byte{'c'};
inline constexpr auto FrameTypeCmdResponse        = std::byte{'C'};
inline constexpr auto FrameTypeServerInfoRequest  = std::byte{'i'};
inline constexpr auto FrameTypeServerInfoResponse = std::byte{'I'};

namespace detail {

template <bool HasAddrWord>
struct CommandRequestFrame;

template <>
struct CommandRequestFrame<false> {
  uint32_t                   service_id;
  uint32_t                   command_id;
  uint32_t                   request_id;
  std::span<const std::byte> payload;
};

template <>
struct CommandRequestFrame<true> {
  uint32_t                   server_address;
  uint32_t                   service_id;
  uint32_t                   command_id;
  uint32_t                   request_id;
  std::span<const std::byte> payload;
};

template <bool HasAddrWord>
struct ServerInfoRequestFrame;

template <>
struct ServerInfoRequestFrame<false> {
  uint32_t request_id;
};

template <>
struct ServerInfoRequestFrame<true> {
  uint32_t server_address;
  uint32_t request_id;
};

}   // namespace detail

/**
 * Represents a vRPC command frame
 * @tparam FF Frame format
 */
template <FrameFormat FF>
using CommandRequestFrame =
    detail::CommandRequestFrame<FF.has_server_addr_word>;

template <FrameFormat FF>
using CommandRequestFrameRef =
    std::reference_wrapper<const CommandRequestFrame<FF>>;

template <FrameFormat FF>
using ServerInfoRequestFrame =
    detail::ServerInfoRequestFrame<FF.has_server_addr_word>;

template <FrameFormat FF>
using ServerInfoRequestRef =
    std::reference_wrapper<const ServerInfoRequestFrame<FF>>;

}   // namespace vrpc