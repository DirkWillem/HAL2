export module modbus.encoding.rtu;

export import :decoder;
export import :encoder;

namespace modbus::encoding::rtu {

export struct Encoding {
  using Decoder = Decoder;
  using Encoder = Encoder;
};

}   // namespace modbus::encoding::rtu