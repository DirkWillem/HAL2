module;

#include <cstdint>

export module modbus.encoding.rtu:frames;

import modbus.core;

namespace modbus::encoding::rtu {

export template <FrameVariant FV>
struct RequestFrame {
  RequestPdu<FV> pdu;
  uint8_t        address;
};

export template <FrameVariant FV>
struct ResponseFrame {
  ResponsePdu<FV> pdu;
  uint8_t         address;
};

}   // namespace modbus::encoding::rtu
