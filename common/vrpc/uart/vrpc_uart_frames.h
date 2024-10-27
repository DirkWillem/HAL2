#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace vrpc {

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

/**
 * Represents a vRPC command frame
 */
struct CommandRequestFrame {
  uint32_t                   service_id;
  uint32_t                   command_id;
  uint32_t                   request_id;
  std::span<const std::byte> payload;
};

using CommandRequestFrameRef =
    std::reference_wrapper<const CommandRequestFrame>;

struct ServerInfoRequestFrame {
  uint32_t request_id;
};

using ServerInfoRequestRef =
    std::reference_wrapper<const ServerInfoRequestFrame>;

}   // namespace vrpc