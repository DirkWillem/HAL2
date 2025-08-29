module;

#include <array>
#include <chrono>
#include <variant>

export module modbus.server.rtos;

import hal.abstract;

import rtos.concepts;

import modbus.core;
export import modbus.server;
import modbus.encoding;
import modbus.encoding.rtu;

export import :helpers;

namespace modbus::server::rtos {

export template <::rtos::concepts::Rtos OS, concepts::Server Srv,
                 hal::RtosUart          Uart,
                 encoding::UartEncoding E = encoding::rtu::Encoding>
class UartServer
    : public OS::template Task<UartServer<OS, Srv, Uart>, OS::MediumStackSize> {
  using Encoder = typename E::Encoder;
  using Decoder = typename E::Decoder;

 public:
  UartServer(Srv& server, Uart& uart, uint8_t address) noexcept
      : OS::template Task<UartServer<OS, Srv, Uart>,
                          OS::MediumStackSize>{"ModbusServer"}
      , server{server}
      , uart{uart}
      , address{address} {}

  void operator()() noexcept {
    using namespace std::chrono_literals;

    while (!OS::template Task<UartServer<OS, Srv, Uart>,
                              OS::MediumStackSize>::StopRequested()) {
      if (const auto recv = uart.Receive(buffer, 1000ms); recv.has_value()) {
        const auto decode_result = Decoder{*recv}.DecodeRequest();

        if (decode_result.has_value()) {
          const auto request_frame = *decode_result;
          [[maybe_unused]] const auto addr          = E::GetAddress(request_frame);

          if (E::GetAddress(request_frame) != address) {
            continue;
          }

          const auto& request_pdu = E::GetPdu(request_frame);

          ResponsePdu response_pdu{};

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

  uint8_t address;
};

}   // namespace modbus::server::rtos