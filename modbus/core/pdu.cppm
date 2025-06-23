module;

#include <bit>
#include <cstdint>
#include <span>
#include <type_traits>
#include <variant>

export module modbus.core:pdu;

import hstd;

namespace modbus {

export enum class FunctionCode : uint8_t {
  ReadCoils              = 0x01,
  ReadDiscreteInputs     = 0x02,
  ReadHoldingRegisters   = 0x03,
  ReadInputRegisters     = 0x04,
  WriteSingleCoil        = 0x05,
  WriteSingleRegister    = 0x06,
  WriteMultipleCoils     = 0x0F,
  WriteMultipleRegisters = 0x10,
  ErrorResponseBase      = 0x80,
};

export enum class FrameVariant { Encode, Decode };

export template <FrameVariant FV, typename T>
using Array = std::conditional_t < FV == FrameVariant::Encode,
      std::span<T>, hstd::BitCastSpan < T, std::byte, std::endian::big >> ;

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

export enum class CoilState : uint16_t {
  Disabled = 0x0000,
  Enabled  = 0xFF00,
};

export struct ReadCoilsRequest {
  static constexpr auto FC = FunctionCode::ReadCoils;

  uint16_t starting_addr;
  uint16_t num_coils;
};

export struct ReadDiscreteInputsRequest {
  static constexpr auto FC = FunctionCode::ReadDiscreteInputs;
  uint16_t              starting_addr;
  uint16_t              num_inputs;
};

export struct ReadHoldingRegistersRequest {
  static constexpr auto FC = FunctionCode::ReadHoldingRegisters;
  uint16_t              starting_addr;
  uint16_t              num_holding_registers;
};

export struct ReadInputRegistersRequest {
  static constexpr auto FC = FunctionCode::ReadInputRegisters;
  uint16_t              starting_addr;
  uint16_t              num_input_registers;
};

export struct WriteSingleCoilRequest {
  static constexpr auto FC = FunctionCode::WriteSingleCoil;
  uint16_t              coil_addr;
  CoilState             new_state;
};

export struct WriteSingleRegisterRequest {
  static constexpr auto FC = FunctionCode::WriteSingleRegister;

  uint16_t register_addr;
  uint16_t new_value;
};

export struct WriteMultipleCoilsRequest {
  static constexpr auto FC = FunctionCode::WriteMultipleCoils;

  uint16_t                   start_addr;
  uint16_t                   num_coils;
  std::span<const std::byte> values;
};

export struct WriteMultipleRegistersRequest {
  static constexpr auto FC = FunctionCode::WriteMultipleRegisters;

  uint16_t                   start_addr;
  uint16_t                   num_registers;
  std::span<const std::byte> values;
};

export template <FrameVariant FV>
using RequestPdu =
    std::variant<ReadCoilsRequest, ReadDiscreteInputsRequest,
                 ReadHoldingRegistersRequest, ReadInputRegistersRequest,
                 WriteSingleCoilRequest, WriteSingleRegisterRequest,
                 WriteMultipleCoilsRequest, WriteMultipleRegistersRequest>;

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

export constexpr ErrorResponse IllegalDataValue(FunctionCode fc) {
  return MakeErrorResponse(fc, ExceptionCode::IllegalDataValue);
}

export struct ReadCoilsResponse {
  static constexpr auto FC = FunctionCode::ReadCoils;

  std::span<const std::byte> coils;
};

export struct ReadDiscreteInputsResponse {
  static constexpr auto FC = FunctionCode::ReadDiscreteInputs;

  std::span<const std::byte> inputs;
};

export struct ReadHoldingRegistersResponse {
  static constexpr auto FC = FunctionCode::ReadHoldingRegisters;

  std::span<const std::byte> registers;
};

export template <FrameVariant FV>
struct ReadInputRegistersResponse {
  static constexpr auto FC = FunctionCode::ReadInputRegisters;

  Array<FV, uint16_t> registers;
};

export struct WriteSingleCoilResponse {
  static constexpr auto FC = FunctionCode::WriteSingleCoil;
  uint16_t              coil_addr;
  CoilState             new_state;
};

export struct WriteSingleRegisterResponse {
  static constexpr auto FC = FunctionCode::WriteSingleRegister;

  uint16_t register_addr;
  uint16_t new_value;
};

export struct WriteMultipleCoilsResponse {
  static constexpr auto FC = FunctionCode::WriteMultipleCoils;

  uint16_t start_addr;
  uint16_t num_coils;
};

export struct WriteMultipleRegistersResponse {
  static constexpr auto FC = FunctionCode::WriteMultipleRegisters;

  uint16_t start_addr;
  uint16_t num_registers;
};

export template <FrameVariant FV>
using ResponsePdu =
    std::variant<ErrorResponse, ReadCoilsResponse, ReadDiscreteInputsResponse,
                 ReadHoldingRegistersResponse, ReadInputRegistersResponse<FV>,
                 WriteSingleCoilResponse, WriteSingleRegisterResponse,
                 WriteMultipleCoilsResponse, WriteMultipleRegistersResponse>;

}   // namespace modbus