#pragma once

#include <halstd/mp/type_helpers.h>

#include "vrpc_uart_client.h"

namespace vrpc::uart {

template <hal::AsyncUart Uart, hal::System Sys, VrpcNetworkConfig NC,
          VrpcUartClientOptions O, typename... ResponseMsgs>
class VrpcUartServiceClient {
  using UartClient = VrpcUartClient<Uart, Sys, NC, O>;

 public:
  void HandleResponses() { uart_client.HandleResponses(); }

 protected:
  static constexpr auto UsesAddressing = UartClient::UsesAddressing;

  explicit VrpcUartServiceClient(
      VrpcUartClient<Uart, Sys, NC, O>& uart_client) noexcept
      : uart_client{uart_client}
      , req_cb{this} {}

  template <typename Req, typename Res>
    requires(!UsesAddressing)
  void
  Request(uint32_t svc_id, uint32_t cmd_id, const Req& req,
          halstd::Callback<const Res&>& callback,
          std::optional<std::reference_wrapper<halstd::Callback<RequestError>>>
              err_callback = {}) noexcept {
    success_callback = &callback;
    error_callback   = err_callback.transform([](auto cb) { return &cb.get(); })
                         .value_or(nullptr);

    req_cb.RebindUnguarded(&VrpcUartServiceClient::RequestCallback<Res>);

    uart_client.template Request<Req>(svc_id, cmd_id, req, req_cb);
  }

  template <typename Req, typename Res>
    requires(UsesAddressing)
  void
  Request(uint32_t server_address, uint32_t svc_id, uint32_t cmd_id,
          const Req& req, halstd::Callback<const Res&>& callback,
          std::optional<std::reference_wrapper<halstd::Callback<RequestError>>>
              err_callback = {}) noexcept {
    success_callback = &callback;
    error_callback   = err_callback.transform([](auto cb) { return &cb.get(); })
                         .value_or(nullptr);

    req_cb.RebindUnguarded(&VrpcUartServiceClient::RequestCallback<Res>);

    uart_client.template Request<Req>(server_address, svc_id, cmd_id, req,
                                      req_cb);
  }

 private:
  template <typename Res>
  void RequestCallback(
      std::expected<std::span<const std::byte>, vrpc::uart::RequestError>
          response) noexcept {
    using SuccessCb = halstd::Callback<const Res&>;
    if (!std::holds_alternative<const SuccessCb*>(success_callback)) {
      return;
    }

    const auto callback = std::get<const SuccessCb*>(success_callback);
    if (response.has_value()) {
      Res res{};
      if (vrpc::ProtoDecode(response.value(), res)) {
        if (callback != nullptr) {
          (*callback)(res);
        }
      } else {
        InvokeErrorCallback(vrpc::uart::RequestError::DecodeFailed);
      }
    } else {
      InvokeErrorCallback(response.error());
    }

    success_callback = std::monostate{};
    error_callback   = nullptr;
  }

  void InvokeErrorCallback(vrpc::uart::RequestError error) const noexcept {
    if (error_callback != nullptr) {
      (*error_callback)(error);
    }
  }

  VrpcUartClient<Uart, Sys, NC, O>& uart_client;

  halstd::DynamicMethodCallback<
      VrpcUartServiceClient,
      std::expected<std::span<const std::byte>, RequestError>>
      req_cb;

  std::variant<std::monostate, const halstd::Callback<const ResponseMsgs&>*...>
      success_callback{std::monostate{}};

  halstd::Callback<RequestError>* error_callback{nullptr};
};

}   // namespace vrpc::uart