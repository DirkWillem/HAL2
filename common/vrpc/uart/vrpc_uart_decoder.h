#pragma once

#include <bit>
#include <optional>
#include <span>
#include <variant>

#include <constexpr_tools/crc.h>
#include <constexpr_tools/spans.h>

#include "vrpc_uart_frames.h"

namespace vrpc {

class UartDecoder {
 private:
  /**
   * Consumes N bytes from the input buffer
   * @param n Number of bytes to consume
   */
  constexpr void Consume(std::size_t n) noexcept {
    if (unparsed_buffer.size() < n) {
      std::unreachable();
    }

    unparsed_buffer = unparsed_buffer.subspan(n);
    read_pos += n;
  }

  /**
   * Reads bytes from the input buffer into the given variable
   * @tparam T Variable type
   * @param into Destination variable
   * @return Whether enough bytes were available to be read
   */
  template <typename T>
  [[nodiscard]] constexpr bool ReadInto(T& into) noexcept {
    // Validate that there are enough bytes to read
    if (unparsed_buffer.size() < sizeof(T)) {
      return false;
    }

    // Copy data into array for use with std::bit_cast
    std::array<std::byte, sizeof(T)> buffer{};
    auto data = unparsed_buffer.subspan(0, sizeof(T));
    std::copy(data.begin(), data.end(), buffer.begin());

    // Read data and consume buffer
    into = std::bit_cast<T>(buffer);
    Consume(sizeof(T));

    return true;
  }

  /**
   * Reads N bytes from the input buffer and returns them as a span of bytes
   * @param n Number of bytes to read
   * @return Optional containing a view over the read bytes if enough bytes were
   *   available, std::nullopt otherwise
   */
  [[nodiscard]] constexpr std::optional<std::span<const std::byte>>
  ReadSpan(std::size_t n) noexcept {
    if (unparsed_buffer.size() < n) {
      return {};
    }

    const auto result = unparsed_buffer.subspan(0, n);
    Consume(n);
    return result;
  }

  /**
   * Attempts to read a value from the buffer
   * @tparam T Type of the variable to read
   * @return The value if enough bytes were available, std::nullopt otherwise
   */
  template <typename T>
    requires(!std::is_same_v<T, std::byte>)
  [[nodiscard]] constexpr std::optional<T> TryRead() noexcept {
    T into{};
    if (ReadInto(into)) {
      return into;
    }
    return {};
  }

  /**
   * Reads a single byte from the input buffer. If no bytes are available,
   * the behavior is undefined
   * @return Byte
   */
  [[nodiscard]] constexpr std::byte ReadByte() noexcept {
    if (unparsed_buffer.empty()) {
      std::unreachable();
    }

    const auto result = unparsed_buffer[0];
    Consume(sizeof(std::byte));
    return result;
  }

  enum class State {
    // Start of frame
    StartOfFrame,
    FrameType,

    // Cmd frame
    CmdServiceId,
    CmdCmdId,
    CmdReqId,
    CmdPayloadLen,
    CmdPayload,

    // End of frame
    Crc,
  };

 public:
  static constexpr std::size_t CmdFrameHeaderLength =
      2 * sizeof(std::byte) + 4 * sizeof(uint32_t);
  static constexpr std::size_t CmdFrameTailLength = sizeof(uint16_t);

  enum class Error {
    InvalidFrameType,
    InvalidCrc,
    UnknownParserState,
  };

  using Result = std::variant<std::monostate, Error, CommandRequestFrameRef>;

  /**
   * Constructor
   * @param full_input_buffer View over the full decoder input buffer
   */
  constexpr explicit UartDecoder(
      std::span<std::byte> full_input_buffer) noexcept
      : full_input_buffer{full_input_buffer} {}

  /**
   * Consumes N bytes from the input buffer
   * @param n_new_bytes Number of new consumable bytes
   * @return Result
   */
  constexpr Result ConsumeBytes(std::size_t n_new_bytes) noexcept {
    // Append new data to unparsed buffer
    if (!unparsed_buffer.empty()) {
      unparsed_buffer = {unparsed_buffer.data(),
                         unparsed_buffer.size() + n_new_bytes};
    } else {
      unparsed_buffer = full_input_buffer.subspan(read_pos, n_new_bytes);
    }

    // Parse data
    while (!unparsed_buffer.empty()) {
      switch (state) {
      case State::StartOfFrame:
        if (ReadByte() == FrameStart) {
          state = State::FrameType;
        }
        break;
      case State::FrameType:
        switch (ReadByte()) {
        case FrameTypeCmdRequest: state = State::CmdServiceId; break;
        default: state = State::StartOfFrame; return Error::InvalidFrameType;
        }
        break;
      case State::CmdServiceId:
        if (ReadInto(cmd_request_frame.service_id)) {
          state = State::CmdCmdId;
        } else {
          return {};
        }
        break;
      case State::CmdCmdId:
        if (ReadInto(cmd_request_frame.command_id)) {
          state = State::CmdReqId;
        } else {
          return {};
        }
        break;
      case State::CmdReqId:
        if (ReadInto(cmd_request_frame.request_id)) {
          state = State::CmdPayloadLen;
        } else {
          return {};
        }
        break;
      case State::CmdPayloadLen:
        if (ReadInto(payload_length)) {
          if (payload_length > 0) {
            state = State::CmdPayload;
          } else {
            state                     = State::Crc;
            cmd_request_frame.payload = {};
          }
        } else {
          return {};
        }
        break;
      case State::CmdPayload: {
        const auto payload_opt = ReadSpan(payload_length);
        if (payload_opt) {
          cmd_request_frame.payload = *payload_opt;
          state                     = State::Crc;
        } else {
          return {};
        }
        break;
      }

      case State::Crc: {
        const auto crc_data = full_input_buffer.subspan(0, read_pos);
        uint16_t   crc_recv{};
        if (ReadInto(crc_recv)) {
          const auto crc_calc = ct::Crc16(crc_data, CrcPoly);

          state = State::StartOfFrame;
          if (crc_calc != crc_recv) {
            return Error::InvalidCrc;
          } else {
            return std::ref(cmd_request_frame);
          }
        } else {
          return {};
        }
        break;
      }

      default: return Error::UnknownParserState;
      }
    }

    return {};
  }

  /**
   * Attempts to decode data from the buffer without consuming any new bytes
   * @return Result
   */
  [[nodiscard]] constexpr Result Decode() noexcept { return ConsumeBytes(0); }

  /**
   * Resets the input buffer
   */
  constexpr void ResetBuffer(std::span<std::byte> full_buffer,
                             std::span<std::byte> unparsed = {}) noexcept {
    full_input_buffer = full_buffer;
    unparsed_buffer   = unparsed;
    read_pos          = 0;
  }

  /**
   * Returns whether the decoder has partially decoded a command
   * @return Whether the decoder has partially decoded a command
   */
  [[nodiscard]] constexpr bool HasPartialCommand() const noexcept {
    return state != State::StartOfFrame;
  }

  /**
   * Returns whether the input buffer is empty
   * @return Whether the input buffer is empty
   */
  [[nodiscard]] constexpr bool buffer_empty() const noexcept {
    return unparsed_buffer.empty();
  }

  [[nodiscard]] constexpr std::span<const std::byte>
  undecoded_buffer() const noexcept {
    return unparsed_buffer;
  }

  [[nodiscard]] constexpr std::span<std::byte> empty_buffer() {
    return full_input_buffer.subspan(read_pos + unparsed_buffer.size());
  }

 private:
  std::size_t          read_pos{0};
  std::span<std::byte> full_input_buffer;
  std::span<std::byte> unparsed_buffer{};
  State                state{State::StartOfFrame};

  uint32_t            payload_length{};
  CommandRequestFrame cmd_request_frame{};
};

}   // namespace vrpc