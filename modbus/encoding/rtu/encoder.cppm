module;

#include <bit>
#include <memory>
#include <span>
#include <utility>

export module modbus.encoding.rtu:encoder;

import hstd;

import modbus.core;

import :frames;

namespace modbus::encoding::rtu {

export class Encoder {
 public:
  constexpr Encoder(uint8_t address, std::span<std::byte> destination) noexcept
      : destination{destination}
      , address{address} {}

  /** Encodes a MODBUS Error response frame */
  constexpr std::span<const std::byte> operator()(const ErrorResponse& frame) {
    Write(address);
    Write(frame.function_code);
    Write(frame.exception_code);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Coils request frame */
  constexpr std::span<const std::byte>
  operator()(const ReadCoilsRequest& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(frame.starting_addr);
    Write(frame.num_coils);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Coils response frame */
  constexpr std::span<const std::byte>
  operator()(const ReadCoilsResponse& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(static_cast<uint8_t>(frame.coils.size()));
    Write(frame.coils);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Discrete Inputs request frame */
  constexpr std::span<const std::byte>
  operator()(const ReadDiscreteInputsRequest& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(frame.starting_addr);
    Write(frame.num_inputs);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Discrete Inputs response frame */
  constexpr std::span<const std::byte>
  operator()(const ReadDiscreteInputsResponse& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(static_cast<uint8_t>(frame.inputs.size()));
    Write(frame.inputs);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Holding Registers request frame */
  constexpr std::span<const std::byte>
  operator()(const ReadHoldingRegistersRequest& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(frame.starting_addr);
    Write(frame.num_holding_registers);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Holding Registers response frame */
  constexpr std::span<const std::byte>
  operator()(const ReadHoldingRegistersResponse<FrameVariant::Encode>&
                 frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(static_cast<uint8_t>(frame.registers.size() * sizeof(uint16_t)));
    Write(frame.registers);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Input Registers request frame */
  constexpr std::span<const std::byte>
  operator()(const ReadInputRegistersRequest& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(frame.starting_addr);
    Write(frame.num_input_registers);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Read Input Registers response frame */
  constexpr std::span<const std::byte> operator()(
      const ReadInputRegistersResponse<FrameVariant::Encode>& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(static_cast<uint8_t>(frame.registers.size() * sizeof(uint16_t)));
    Write(frame.registers);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Write Single Coil request frame */
  constexpr std::span<const std::byte>
  operator()(const WriteSingleCoilRequest& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(frame.coil_addr);
    Write(frame.new_state);
    WriteCrc();

    return Written();
  }

  /** Encodes a MODBUS Write Single Coil request frame */
  constexpr std::span<const std::byte>
  operator()(const WriteSingleCoilResponse& frame) noexcept {
    Write(address);
    Write(frame.FC);
    Write(frame.coil_addr);
    Write(frame.new_state);
    WriteCrc();

    return Written();
  }

 private:
  constexpr void Write(auto v) noexcept
    requires std::is_enum_v<std::decay_t<decltype(v)>>
  {
    Write(std::to_underlying(v));
  }

  constexpr void Write(std::unsigned_integral auto v) noexcept {
    const auto tmp = hstd::ConvertToEndianness<std::endian::big>(v);
    std::memcpy(destination.subspan(offset, sizeof(tmp)).data(), &tmp,
                sizeof(tmp));

    offset += sizeof(tmp);
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  constexpr void Write(std::span<T> data) noexcept {
    if constexpr (sizeof(T) > 1 && std::is_unsigned_v<T>) {
      for (std::size_t i = 0; i < data.size(); ++i) {
        const auto tmp = hstd::ConvertToEndianness<std::endian::big>(data[i]);
        std::memcpy(
            destination.subspan(offset + i * sizeof(T), sizeof(T)).data(), &tmp,
            sizeof(T));
      }
    } else {
      const auto sz = data.size() * sizeof(T);
      std::memcpy(destination.subspan(offset, sz).data(),
                  hstd::ReinterpretSpan<std::byte>(data).data(), sz);
    }

    offset += data.size_bytes();
  }

  constexpr void WriteCrc() noexcept {
    Write(hstd::Crc16(destination.subspan(0, offset), 0xA001, 0xFFFF));
  }

  constexpr std::span<const std::byte> Written() noexcept {
    return destination.subspan(0, offset);
  }

  std::span<std::byte> destination;
  uint8_t              address;
  std::size_t          offset{0};
};

}   // namespace modbus::encoding::rtu
