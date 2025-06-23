module;

#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <optional>
#include <span>

export module modbus.encoding.rtu:decoder;

import hstd;

import modbus.core;

import :frames;

namespace modbus::encoding::rtu {

export enum class DecodeError {
  InvalidFunctionCode,
  IncompleteCommand,
  InvalidCrc,
  TooMuchData,
  ByteCountNotAMultipleOfDataSize,
  UnknownError,
};

struct LengthPrefixedBytes {
  uint8_t*                    length;
  std::span<const std::byte>* bytes;
  std::size_t                 element_size = 1;
};

export class Decoder {
  static constexpr auto FV = FrameVariant::Decode;

 public:
  using ReqFrame = RequestFrame<FV>;
  using ResFrame = ResponseFrame<FV>;
  using Error    = DecodeError;

  constexpr explicit Decoder(std::span<const std::byte> buffer)
      : buffer{buffer} {}

  constexpr std::expected<RequestFrame<FV>, DecodeError>
  DecodeRequest() noexcept {
    // Minimal frame length is 4 bytes (address, function code, 2 CRC bytes)
    if (buffer.size() < 4) {
      return std::unexpected(DecodeError::IncompleteCommand);
    }

    // Decode depending on function code
    const auto function_code = static_cast<FunctionCode>(buffer[1]);
    switch (function_code) {
    case FunctionCode::ReadCoils:
      return DecodeRequestPayload<ReadCoilsRequest>([this](auto& req) {
        return DecodeVars(req.starting_addr, req.num_coils);
      });
    case FunctionCode::ReadDiscreteInputs:
      return DecodeRequestPayload<ReadDiscreteInputsRequest>([this](auto& req) {
        return DecodeVars(req.starting_addr, req.num_inputs);
      });
    case FunctionCode::ReadHoldingRegisters:
      return DecodeRequestPayload<ReadHoldingRegistersRequest>(
          [this](auto& req) {
            return DecodeVars(req.starting_addr, req.num_holding_registers);
          });
    case FunctionCode::ReadInputRegisters:
      return DecodeRequestPayload<ReadInputRegistersRequest>([this](auto& req) {
        return DecodeVars(req.starting_addr, req.num_input_registers);
      });
    case FunctionCode::WriteSingleCoil:
      return DecodeRequestPayload<WriteSingleCoilRequest>([this](auto& req) {
        return DecodeVars(req.coil_addr, req.new_state);
      });
    case FunctionCode::WriteSingleRegister:
      return DecodeRequestPayload<WriteSingleRegisterRequest>(
          [this](auto& req) {
            return DecodeVars(req.register_addr, req.new_value);
          });
    case FunctionCode::WriteMultipleCoils:
      return DecodeRequestPayload<WriteMultipleCoilsRequest>([this](auto& req) {
        return DecodeVars(
            req.start_addr, req.num_coils,
            LengthPrefixedBytes{.length = nullptr, .bytes = &req.values});
      });
    case FunctionCode::WriteMultipleRegisters:
      return DecodeRequestPayload<WriteMultipleRegistersRequest>(
          [this](auto& req) {
            return DecodeVars(
                req.start_addr, req.num_registers,
                LengthPrefixedBytes{.length = nullptr, .bytes = &req.values});
          });
    default: return std::unexpected(DecodeError::InvalidFunctionCode);
    }
  }

  constexpr std::expected<ResponseFrame<FV>, DecodeError>
  DecodeResponse() noexcept {
    // Minimal frame length is 4 bytes (address, function code, 2 CRC bytes)
    if (buffer.size() < 4) {
      return std::unexpected(DecodeError::IncompleteCommand);
    }

    // Handle error responses
    const auto function_code_num = static_cast<uint8_t>(buffer[1]);
    if (function_code_num
        >= static_cast<uint8_t>(FunctionCode::ErrorResponseBase)) {
      return DecodeResponsePayload<ErrorResponse>(
          [this, function_code_num](auto& res) {
            res.function_code = function_code_num;
            return DecodeVars(res.exception_code);
          });
    }

    // Decode depending on function code
    const auto function_code = static_cast<FunctionCode>(buffer[1]);
    switch (function_code) {
    case FunctionCode::ReadCoils:
      return DecodeResponsePayload<ReadCoilsResponse>([this](auto& res) {
        return DecodeVars(LengthPrefixedBytes{
            .length = nullptr,
            .bytes  = &res.coils,
        });
      });
    case FunctionCode::ReadDiscreteInputs:
      return DecodeResponsePayload<ReadDiscreteInputsResponse>(
          [this](auto& res) {
            return DecodeVars(LengthPrefixedBytes{
                .length = nullptr,
                .bytes  = &res.inputs,
            });
          });
    case FunctionCode::ReadHoldingRegisters:
      return DecodeResponsePayload<ReadHoldingRegistersResponse>(
          [this](auto& res) {
            return DecodeVars(LengthPrefixedBytes{
                .length = nullptr,
                .bytes  = &res.registers,
            });
          });
    case FunctionCode::ReadInputRegisters:
      return DecodeResponsePayload<ReadInputRegistersResponse<FV>>(
          [this](auto& res) { return DecodeVars(res.registers); });
    case FunctionCode::WriteSingleCoil:
      return DecodeResponsePayload<WriteSingleCoilResponse>([this](auto& res) {
        return DecodeVars(res.coil_addr, res.new_state);
      });
    case FunctionCode::WriteSingleRegister:
      return DecodeResponsePayload<WriteSingleRegisterResponse>(
          [this](auto& res) {
            return DecodeVars(res.register_addr, res.new_value);
          });
    case FunctionCode::WriteMultipleCoils:
      return DecodeResponsePayload<WriteMultipleCoilsResponse>(
          [this](auto& res) {
            return DecodeVars(res.start_addr, res.num_coils);
          });
    case FunctionCode::WriteMultipleRegisters:
      return DecodeResponsePayload<WriteMultipleRegistersResponse>(
          [this](auto& res) {
            return DecodeVars(res.start_addr, res.num_registers);
          });
    default: return std::unexpected(DecodeError::InvalidFunctionCode);
    }
  }

 private:
  template <typename T>
  constexpr std::expected<RequestFrame<FV>, DecodeError>
  DecodeRequestPayload(std::invocable<T&> auto decode) noexcept
    requires std::convertible_to<
        std::invoke_result_t<std::decay_t<decltype(decode)>, T&>,
        std::optional<DecodeError>>
  {
    return DecodePayload<RequestFrame, T>(decode);
  }

  template <typename T>
  constexpr std::expected<ResponseFrame<FV>, DecodeError>
  DecodeResponsePayload(std::invocable<T&> auto decode) noexcept
    requires std::convertible_to<
        std::invoke_result_t<std::decay_t<decltype(decode)>, T&>,
        std::optional<DecodeError>>
  {
    return DecodePayload<ResponseFrame, T>(decode);
  }

  template <template <FrameVariant> typename Frame, typename T>
  constexpr std::expected<Frame<FV>, DecodeError>
  DecodePayload(std::invocable<T&> auto decode) noexcept
    requires std::convertible_to<
        std::invoke_result_t<std::decay_t<decltype(decode)>, T&>,
        std::optional<DecodeError>>
  {
    // Decode payload
    T request{};
    if (const auto err = decode(request); err.has_value()) {
      return std::unexpected(*err);
    }

    // Validate CRC
    const auto crc_calc =
        hstd::Crc16(buffer.subspan(0, buffer.size() - 2), 0xA001, 0xFFFF);
    uint16_t crc_recv{};
    std::memcpy(
        &crc_recv,
        buffer.subspan(buffer.size() - sizeof(crc_recv), sizeof(crc_recv))
            .data(),
        sizeof(crc_recv));

    if (crc_calc != crc_recv) {
      return std::unexpected(DecodeError::InvalidCrc);
    }

    // Construct received frame
    const auto addr = static_cast<uint8_t>(buffer[0]);

    return Frame<FV>{
        .pdu     = {request},
        .address = addr,
    };
  }

  template <typename... Ts>
  std::optional<DecodeError> DecodeVars(Ts&&... vars) noexcept {
    auto buffer = FrameDataBuffer();

    std::optional<DecodeError> err{};
    if (!(DecodeVar(buffer, err, std::forward<Ts>(vars)) && ...)) {
      return err.value_or(DecodeError::IncompleteCommand);
    }

    if (buffer.size() != 0) {
      return {DecodeError::TooMuchData};
    }

    return std::nullopt;
  }

  template <typename T>
  bool DecodeVar(std::span<const std::byte>&                  buffer,
                 [[maybe_unused]] std::optional<DecodeError>& err,
                 T&                                           into) noexcept
    requires(std::unsigned_integral<T>
             || (std::is_enum_v<T>
                 && std::unsigned_integral<std::underlying_type_t<T>>))
  {
    if (buffer.size() < sizeof(T)) {
      return false;
    }

    T tmp;
    std::memcpy(&tmp, buffer.data(), sizeof(T));

    if constexpr (std::is_enum_v<T>) {
      into = static_cast<T>(hstd::ConvertFromEndianness<std::endian::big>(
          static_cast<std::underlying_type_t<T>>(tmp)));

    } else {
      into = hstd::ConvertFromEndianness<std::endian::big>(tmp);
    }
    buffer = buffer.subspan(sizeof(T));

    return true;
  }

  bool DecodeVar(std::span<const std::byte>&                  buffer,
                 [[maybe_unused]] std::optional<DecodeError>& err,
                 LengthPrefixedBytes                          bytes) noexcept {
    if (buffer.size() < 1) {
      return false;
    }

    const auto count = static_cast<uint8_t>(buffer[0]) * bytes.element_size;

    if (bytes.length != nullptr) {
      *bytes.length = count;
    }

    if (buffer.size() < count + 1) {
      return false;
    }

    *bytes.bytes = buffer.subspan(1, count);
    buffer       = buffer.subspan(count + 1);

    return true;
  }

  template <typename T, std::endian E>
  bool DecodeVar(std::span<const std::byte>&                  buffer,
                 [[maybe_unused]] std::optional<DecodeError>& err,
                 hstd::BitCastSpan<T, std::byte, E>&          array) {
    if (buffer.size() < 1) {
      return false;
    }

    const auto byte_count = static_cast<uint8_t>(buffer[0]);
    if (buffer.size() < byte_count + 1) {
      return false;
    }

    array  = Array<FV, T>(buffer.subspan(1, byte_count));
    buffer = buffer.subspan(byte_count + 1);

    return true;
  }

  std::span<const std::byte> FrameDataBuffer() noexcept {
    return buffer.subspan(2, buffer.size() - 4);
  }

  std::span<const std::byte> buffer;
};

}   // namespace modbus::encoding::rtu