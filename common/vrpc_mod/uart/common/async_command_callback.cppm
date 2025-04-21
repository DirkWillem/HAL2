module;

#include <optional>
#include <span>

export module vrpc.uart.common:async_command_callback;

import hstd;

import vrpc.server;

namespace vrpc::uart {

export enum class HandleState {
  Handled,
  HandlingAsync,
  ErrUnknownCommand,
  ErrMalformedPayload,
  ErrEncodeFailure,
};

export struct HandleResult {
  HandleState                state;
  std::span<const std::byte> response_payload;
};

export template <bool Enable, typename Response>
class AsyncCommandCallback;

template <typename Response>
class AsyncCommandCallback<false, Response> {};

template <typename Response>
class AsyncCommandCallback<true, Response> {
 public:
  AsyncResult<Response> InitializeCallback(
      std::span<std::byte>          new_response_buf,
      hstd::Callback<HandleResult>& new_inner_callback) & noexcept {
    response_buf   = new_response_buf;
    inner_callback = &new_inner_callback;

    return AsyncResult{response, callback};
  }

 protected:
  AsyncCommandCallback() noexcept
      : callback{this, &AsyncCommandCallback::CommandCallback} {}

  void CommandCallback() noexcept {
    if (inner_callback != nullptr) {
      const auto [enc_success, enc_data] =
          vrpc::ProtoEncode(response, *response_buf);
      if (!enc_success) {
        (*inner_callback)({
            .state            = HandleState::ErrMalformedPayload,
            .response_payload = {},
        });
      } else {
        (*inner_callback)({
            .state            = HandleState::Handled,
            .response_payload = enc_data,
        });
      }

      response_buf   = std::nullopt;
      inner_callback = nullptr;
    }
  }

 private:
  Response                            response{};
  std::optional<std::span<std::byte>> response_buf{};

  hstd::MethodCallback<AsyncCommandCallback<true, Response>> callback;
  hstd::Callback<HandleResult>* inner_callback{nullptr};
};

}   // namespace vrpc::uart