module;

#include <cstdint>
#include <expected>
#include <variant>

export module modbus.encoding;

import modbus.core;

namespace modbus::encoding {

export template <typename Enc>
concept Encoder = requires(Enc enc) {
  std::visit(enc, std::declval<RequestPdu<FrameVariant::Encode>>());
  std::visit(enc, std::declval<ResponsePdu<FrameVariant::Encode>>());
};

export template <typename Dec>
concept Decoder = requires(Dec dec) {
  typename Dec::ReqFrame;
  typename Dec::ResFrame;
  typename Dec::Error;

  {
    dec.DecodeRequest()
  } -> std::convertible_to<
      std::expected<typename Dec::ReqFrame, typename Dec::Error>>;

  {
    dec.DecodeResponse()
  } -> std::convertible_to<
      std::expected<typename Dec::ResFrame, typename Dec::Error>>;
};

export template <typename E>
concept Encoding = requires {
  requires Encoder<typename E::Encoder>;
  requires Decoder<typename E::Decoder>;
};

export template <typename E>
concept UartEncoding = Encoding<E> && requires {
  {
    E::GetAddress(std::declval<const typename E::Decoder::ReqFrame&>())
  } -> std::convertible_to<uint8_t>;
  {
    E::GetPdu(std::declval<const typename E::Decoder::ReqFrame&>())
  } -> std::convertible_to<const RequestPdu<FrameVariant::Decode>&>;
};

}   // namespace modbus::encoding