module;

#include <cstdint>

export module modbus.encoding.rtu:frames;

import modbus.core;

namespace modbus::encoding::rtu {

export struct RequestFrame {
  RequestPdu pdu;
  uint8_t    address;
};

export struct ResponseFrame {
  ResponsePdu pdu;
  uint8_t     address;
};

}   // namespace modbus::encoding::rtu
