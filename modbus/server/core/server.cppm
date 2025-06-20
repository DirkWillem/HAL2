module;

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <variant>

export module modbus.server;

import hstd;

export import :coil;
export import :holding_register;
import :server_storage;

namespace modbus::server {

export template <typename Cs, typename HRs>
class Server : public ServerStorage<Cs, HRs> {
  class FrameHandler {
    template <typename T, T Div>
    static constexpr T DivCeil(T lhs) {
      return lhs % Div == 0 ? lhs / Div : lhs / Div + 1;
    }

   public:
    constexpr FrameHandler(Server&                            server,
                           ResponsePdu<FrameVariant::Encode>& response)
        : server{server}
        , response{response}
        , buffer{server.buffer} {}

    /**
     * Handles a Read Coils request
     * @param req Request to handle
     */
    void operator()(const ReadCoilsRequest& req) noexcept {
      const auto n_bytes = DivCeil<uint16_t, 8>(req.num_coils);

      bool any_read = false;

      for (uint32_t i = 0; i < n_bytes; i++) {
        const auto n_bits_i = std::min(8U, req.num_coils - i * 8);
        const auto result   = server.template ReadCoils<uint8_t>(
            req.starting_addr + i * 8, n_bits_i, any_read);

        if (result) {
          buffer[i] = static_cast<std::byte>(*result);
          any_read  = true;
        } else {
          response = MakeErrorResponse(req.FC, result.error());
          return;
        }
      }

      response = ReadCoilsResponse{.coils = buffer.subspan(0, n_bytes)};
    }

    /**
     * Handles a MODBUS Read Holding Registers frame
     * @param req Request to handle
     */
    void operator()(const ReadHoldingRegistersRequest& req) noexcept {
      const auto result =
          server.template ReadHoldingRegisters<std::endian::big>(
              buffer, req.starting_addr, req.num_holding_registers);

      if (!result.has_value()) {
        response = MakeErrorResponse(req.FC, result.error());
        return;
      }

      response = ReadHoldingRegistersResponse{.registers = result.value()};
    }

    /**
     * Handles a MODBUS Write Single Coil request
     * @param req Request to handle
     */
    void operator()(const WriteSingleCoilRequest& req) noexcept {
      using enum CoilState;

      if (req.new_state != Disabled && req.new_state != Enabled) {
        response = IllegalDataValue(WriteSingleCoilRequest::FC);
        return;
      }

      const auto result =
          server.WriteCoil(req.coil_addr, req.new_state == Enabled);
      if (result) {
        response = WriteSingleCoilResponse{.coil_addr = req.coil_addr,
                                           .new_state = req.new_state};
      } else {
        response = MakeErrorResponse(req.FC, result.error());
      }
    }

    /**
     * Handles a MODBUS write multiple coils request
     * @param req Request to handle
     */
    void operator()(const WriteMultipleCoilsRequest& req) noexcept {
      // Validate that the number of coils to write matches the amount of bytes
      if (req.num_coils < 8 * (req.values.size() - 1) + 1
          || req.num_coils > req.values.size() * 8) {
        response = IllegalDataValue(WriteMultipleCoilsRequest::FC);
        return;
      }

      // Write coils
      const auto result =
          server.WriteCoils(req.start_addr, req.num_coils, req.values);
      if (!result.has_value()) {
        response = MakeErrorResponse(req.FC, result.error());
        return;
      }

      // Create response
      response = WriteMultipleCoilsResponse{
          .start_addr = req.start_addr,
          .num_coils  = req.num_coils,
      };
    }

    void operator()(const auto&) noexcept {}

   private:
    Server&                            server;
    ResponsePdu<FrameVariant::Encode>& response;

    std::span<std::byte> buffer;
  };

 public:
  void HandleFrame(const RequestPdu<FrameVariant::Decode>& request,
                   ResponsePdu<FrameVariant::Encode>&      response) {
    std::visit(FrameHandler{*this, response}, request);
  }

 private:
  std::array<std::byte, 256> buffer{};
};

namespace concepts {
template <typename T>
inline constexpr bool IsServer = false;

template <typename UC, typename UHR>
inline constexpr bool IsServer<Server<UC, UHR>> = true;

export template <typename T>
concept Server = IsServer<T>;

}   // namespace concepts

}   // namespace modbus::server