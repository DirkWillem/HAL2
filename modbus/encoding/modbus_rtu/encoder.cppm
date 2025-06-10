module;

#include <bit>
#include <memory>
#include <span>
#include <utility>

export module modbus.encoding.rtu:encoder;

import hstd;

import modbus.encoding;

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

  template <hstd::ByteLike T>
  constexpr void Write(std::span<const T> data) noexcept {
    const auto sz = data.size();
    std::memcpy(destination.subspan(offset, sz).data(),
                hstd::ReinterpretSpan<std::byte>(data).data(), sz);

    offset += sz;
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
