#pragma once

#include <expected>

#include <halstd/atomic_helpers.h>

#include <hal/system.h>
#include <hal/uart.h>

#include <vrpc/proto_helpers.h>
#include <vrpc/vrpc.h>

#include "vrpc_uart_decoder.h"
#include "vrpc_uart_encode.h"
#include "vrpc_uart_frames.h"

namespace vrpc::uart {

template <typename O>
concept VrpcUartClientOptions = requires {
  { O::MultiDrop } -> std::convertible_to<bool>;
  { O::BufferSize } -> std::convertible_to<std::size_t>;
};

struct DefaultVrpcUartClientOptions {
  static constexpr bool        MultiDrop  = false;
  static constexpr std::size_t BufferSize = 256;
};

enum class RequestError {
  ServiceIdMismatch,
  CommandIdMismatch,
  RequestIdMismatch,
  ServerAddressMismatch,

  DecodeFailed,

  InternalError
};

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          VrpcUartClientOptions O>
class VrpcUartClient {
  using ResponseResult =
      std::expected<std::span<const std::byte>, RequestError>;
  static_assert(halstd::Implies(
                    O::MultiDrop,
                    NC.topology == NetworkTopology::SingleClientMultipleServer),
                "Multi-drop UART servers are only supported for "
                "multiple-server topologies");
  static_assert(halstd::Implies(
                    NC.topology == NetworkTopology::SingleClientMultipleServer,
                    O::MultiDrop),
                "Multiple-server topology requires an addressing mechanism "
                "such as multi-drop");

 public:
  static constexpr auto UsesAddressing = O::MultiDrop;

 private:
  static constexpr FrameFormat FrameFmt{.has_server_addr_word = UsesAddressing};
  using Decoder = UartDecoder<FrameFmt>;

 public:
  explicit VrpcUartClient(Uart& uart) noexcept
      : uart{uart}
      , raw_rx_callback{this, &VrpcUartClient::RawReceiveCallback}
      , tx_callback{this, &VrpcUartClient::TransmitCallback} {
    uart.RegisterUartReceiveCallback(raw_rx_callback);
    uart.RegisterUartTransmitCallback(tx_callback);

    has_response.clear();
    busy.clear();
  }

  template <typename Req>
    requires(!UsesAddressing)
  void Request(uint32_t service_id, uint32_t command_id, const Req& request,
               halstd::Callback<ResponseResult>& callback) {
    halstd::ExclusiveWithAtomicFlag(busy, [&, this] {
      rx_callback = &callback;

      ++req_id;
      svc_id = service_id;
      cmd_id = command_id;

      const auto [enc_success, payload] =
          ProtoEncode(request, GetCommandFramePayloadBuffer(buffer));

      if (enc_success) {
        const auto cmd_frame = EncodeCommandFrame<FrameFmt>(
            buffer, svc_id, cmd_id, req_id, payload);
        uart.Write(cmd_frame);
      }
    });
  }

  template <typename Req>
    requires(UsesAddressing)
  void Request(uint32_t server_address, uint32_t service_id,
               uint32_t command_id, const Req& request,
               halstd::Callback<ResponseResult>& callback) {
    halstd::ExclusiveWithAtomicFlag(busy, [&, this] {
      rx_callback = &callback;

      ++req_id;
      server_addr = server_address;
      svc_id      = service_id;
      cmd_id      = command_id;

      const auto [enc_success, payload] =
          ProtoEncode(request, GetCommandFramePayloadBuffer(buffer));

      if (enc_success) {
        const auto cmd_frame = EncodeCommandFrame<FrameFmt>(
            buffer, server_address, svc_id, cmd_id, req_id, payload);
        uart.Write(cmd_frame);
      }
    });
  }

  void HandleResponses() noexcept {
    if (busy.test_and_set()) {
      if (has_response.test()) {
        halstd::ClearFlagAtExit clear_busy{busy};
        halstd::ClearFlagAtExit clear_has_response{has_response};

        if (rx_callback != nullptr) {
          if (response.has_value()) {
            (*rx_callback)(*response);
          } else {
            (*rx_callback)(std::unexpected(RequestError::InternalError));
          }
        }
      }
    }
  }

 private:
  [[nodiscard]] static constexpr std::span<std::byte>
  GetCommandFramePayloadBuffer(std::span<std::byte> full_buffer) noexcept {
    auto result = full_buffer.subspan(Decoder::CmdFrameHeaderLength);
    return result.subspan(0, result.size() - Decoder::CmdFrameTailLength);
  }

  void RawReceiveCallback(std::span<std::byte> rx_data) noexcept {
    auto result = decoder.ConsumeBytes(rx_data.size());
    while (!std::holds_alternative<std::monostate>(result)) {
      if (std::holds_alternative<CommandRequestFrameRef<FrameFmt>>(result)) {
        if (rx_callback == nullptr) {
          return;
        }

        if (!has_response.test_and_set()) {
          auto request =
              std::get<CommandRequestFrameRef<FrameFmt>>(result).get();
          if constexpr (UsesAddressing) {
            if (request.server_address != server_addr) {
              response = std::unexpected(RequestError::ServerAddressMismatch);
              return;
            }
          }

          if (request.service_id != svc_id) {
            response = std::unexpected(RequestError::ServiceIdMismatch);
            return;
          }
          if (request.command_id != cmd_id) {
            response = std::unexpected(RequestError::CommandIdMismatch);
            (*rx_callback)(std::unexpected(RequestError::CommandIdMismatch));
            return;
          }
          if (request.request_id != req_id) {
            response = std::unexpected(RequestError::RequestIdMismatch);
            return;
          }

          response = request.payload;
        }
      }

      result = decoder.Decode();
    }
  }
  void TransmitCallback() noexcept { uart.Receive(buffer); }

  Uart& uart;

  typename Sys::AtomicFlag busy{};
  typename Sys::AtomicFlag has_response{};

  uint32_t server_addr{};
  uint32_t svc_id{};
  uint32_t cmd_id{};
  uint32_t req_id{0};

  std::span<const std::byte>           rx_data{};
  std::array<std::byte, O::BufferSize> buffer{};
  Decoder                              decoder{buffer};

  halstd::MethodCallback<VrpcUartClient, std::span<std::byte>> raw_rx_callback;
  halstd::MethodCallback<VrpcUartClient>                       tx_callback;

  std::optional<ResponseResult>     response{};
  halstd::Callback<ResponseResult>* rx_callback{nullptr};
};

}   // namespace vrpc::uart