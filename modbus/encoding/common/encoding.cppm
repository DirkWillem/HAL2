module;

#include <cstdint>
#include <span>
#include <variant>

export module modbus.encoding;

namespace modbus {

export enum class FunctionCode : uint8_t {
  ReadCoils         = 0x01,
  ErrorResponseBase = 0x80,
};

export enum class FrameVariant { Encode, Decode };

export enum class ExceptionCode : uint8_t {
  IllegalFunction                    = 0x01,
  IllegalDataAddress                 = 0x02,
  IllegalDataValue                   = 0x03,
  ServerDeviceFailure                = 0x04,
  Acknowledge                        = 0x05,
  ServerDeviceBusy                   = 0x06,
  MemoryParityError                  = 0x08,
  GatewayPathUnavailable             = 0x0A,
  GatewayTargetDeviceFailedToRespond = 0x0B,
};

export struct ReadCoilsRequest {
  static constexpr auto FC = FunctionCode::ReadCoils;

  uint16_t starting_addr;
  uint16_t num_coils;
};

export template <FrameVariant FV>
using RequestFramePayload = std::variant<ReadCoilsRequest>;

export template <FrameVariant FV>
struct RequestFrame {
  RequestFramePayload<FV> payload;
  uint8_t                 address;
};

export struct ErrorResponse {
  uint8_t       function_code;
  ExceptionCode exception_code;
};

export constexpr ErrorResponse MakeErrorResponse(FunctionCode  fc,
                                                 ExceptionCode ec) {
  return {
      .function_code = static_cast<uint8_t>(
          static_cast<uint8_t>(FunctionCode::ErrorResponseBase)
          + static_cast<uint8_t>(fc)),
      .exception_code = ec,
  };
}

export struct ReadCoilsResponse {
  static constexpr auto FC = FunctionCode::ReadCoils;

  std::span<const std::byte> coils;
};

export template <FrameVariant FV>
using ResponseFramePayload = std::variant<ErrorResponse, ReadCoilsResponse>;

export template <FrameVariant FV>
struct ResponseFrame {
  ResponseFramePayload<FV> payload;
  uint8_t                  address;
};

}   // namespace modbus