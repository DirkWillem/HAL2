module;

#include <cstddef>
#include <cstdint>
#include <span>

export module vrpc.uart.common:frame;

namespace vrpc::uart {

export struct FrameFormat {
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

export inline constexpr uint16_t CrcPoly = 0xA001;

export inline constexpr auto FrameStart                  = std::byte{'V'};
export inline constexpr auto FrameTypeCmdRequest         = std::byte{'c'};
export inline constexpr auto FrameTypeCmdResponse        = std::byte{'C'};
export inline constexpr auto FrameTypeServerInfoRequest  = std::byte{'i'};
export inline constexpr auto FrameTypeServerInfoResponse = std::byte{'I'};

template <bool HasAddrWord>
struct CommandRequestFrameImpl;

template <>
struct CommandRequestFrameImpl<false> {
  uint32_t                   service_id;
  uint32_t                   command_id;
  uint32_t                   request_id;
  std::span<const std::byte> payload;
};

template <>
struct CommandRequestFrameImpl<true> {
  uint32_t                   server_address;
  uint32_t                   service_id;
  uint32_t                   command_id;
  uint32_t                   request_id;
  std::span<const std::byte> payload;
};

template <bool HasAddrWord>
struct ServerInfoRequestFrameImpl;

template <>
struct ServerInfoRequestFrameImpl<false> {
  uint32_t request_id;
};

template <>
struct ServerInfoRequestFrameImpl<true> {
  uint32_t server_address;
  uint32_t request_id;
};

/**
 * Represents a vRPC command frame
 * @tparam FF Frame format
 */
export template <FrameFormat FF>
using CommandRequestFrame =
    CommandRequestFrameImpl<FF.has_server_addr_word>;

export template <FrameFormat FF>
using CommandRequestFrameRef =
    std::reference_wrapper<const CommandRequestFrame<FF>>;

export template <FrameFormat FF>
using ServerInfoRequestFrame =
    ServerInfoRequestFrameImpl<FF.has_server_addr_word>;

export template <FrameFormat FF>
using ServerInfoRequestRef =
    std::reference_wrapper<const ServerInfoRequestFrame<FF>>;

}   // namespace vrpc