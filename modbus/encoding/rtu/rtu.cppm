module;

#include <cstdint>

export module modbus.encoding.rtu;

export import :decoder;
export import :encoder;
export import :frames;

namespace modbus::encoding::rtu {

export struct Encoding {
  using Decoder = Decoder;
  using Encoder = Encoder;

  static constexpr uint8_t
  GetAddress(const RequestFrame<FrameVariant::Decode>& frame) noexcept {
    return frame.address;
  }

  static constexpr RequestPdu<FrameVariant::Decode>
  GetPdu(const RequestFrame<FrameVariant::Decode>& frame) noexcept {
    return frame.pdu;
  }
};

}   // namespace modbus::encoding::rtu