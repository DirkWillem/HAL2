module;

#include <array>
#include <chrono>

export module modbus.server.freertos;

import hal.abstract;

import rtos.freertos;

import modbus.core;
export import modbus.server;
import modbus.encoding;
import modbus.encoding.rtu;

namespace modbus::server::freertos {

export template <concepts::Server Srv, hal::RtosUart Uart,
                 encoding::UartEncoding E = encoding::rtu::Encoding>
class UartServer : public rtos::Task<UartServer<Srv, Uart>, 512> {
  using Encoder = typename E::Encoder;
  using Decoder = typename E::Decoder;

 public:
  UartServer(Srv& server, Uart& uart) noexcept
      : rtos::Task<UartServer, 512>{"ModbusServer"}
      , server{server}
      , uart{uart} {}

  [[noreturn]] void operator()() noexcept {
    using namespace std::chrono_literals;

    while (true) {
      if (const auto recv = uart.Receive(buffer, 1000ms); recv.has_value()) {
        const auto decode_result = Decoder{*recv}.DecodeRequest();

        if (decode_result.has_value()) {
          const auto  request_frame = *decode_result;
          const auto& request_pdu   = E::GetPdu(request_frame);

          ResponsePdu<FrameVariant::Encode> response_pdu{};

          server.HandleFrame(request_pdu, response_pdu);

          const auto encoded_response_frame =
              std::visit(Encoder{0x01, buffer}, response_pdu);

          uart.Write(encoded_response_frame, 100ms);
        }
      }
    }
  }

 private:
  Srv&  server;
  Uart& uart;

  std::array<std::byte, 256> buffer{};
};

}   // namespace modbus::server::freertos