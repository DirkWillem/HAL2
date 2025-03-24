#pragma once

#include "transport.h"

namespace vrpc::uart {

template <hal::AsyncUart Uart, hal::System Sys, NetworkConfig NC,
          ClientTransportOptions O, std::size_t SlotId,
          typename... ResponseMsgs>
class ServiceClientBase {
  using UartClient = ClientTransport<Uart, Sys, NC, O>;

 public:
  using Error = RequestError;

  void HandleResponses() { uart_client.HandleResponses(); }

 protected:
  static constexpr auto UsesAddressing = UartClient::UsesAddressing;

  explicit ServiceClientBase(
      ClientTransport<Uart, Sys, NC, O>& uart_client) noexcept
      : uart_client{uart_client}
      , req_cb{this} {}

  template <typename Req, typename Res>
    requires(!UsesAddressing)
  void Request(
      uint32_t svc_id, uint32_t cmd_id, const Req& req,
      halstd::Callback<std::expected<std::reference_wrapper<const Res>, Error>>&
          callback) noexcept {
    inner_callback = &callback;
    // error_callback   = err_callback.transform([](auto cb) { return &cb.get();
    // })
    //                      .value_or(nullptr);

    req_cb.RebindUnguarded(&ServiceClientBase::RequestCallback<Res>);

    uart_client.template Request<Req>(SlotId, svc_id, cmd_id, req, req_cb);
  }

  template <typename Req, typename Res>
    requires(UsesAddressing)
  void Request(
      uint32_t server_address, uint32_t svc_id, uint32_t cmd_id, const Req& req,
      halstd::Callback<std::expected<std::reference_wrapper<const Res>, Error>>&
          callback) noexcept {
    inner_callback = &callback;
    // success_callback = &callback;
    // error_callback   = err_callback.transform([](auto cb) { return &cb.get();
    // })
    //                      .value_or(nullptr);

    req_cb.RebindUnguarded(&ServiceClientBase::RequestCallback<Res>);

    uart_client.template Request<Req>(SlotId, server_address, svc_id, cmd_id,
                                      req, req_cb);
  }

 private:
  template <typename Res>
  void RequestCallback(
      std::expected<std::span<const std::byte>, vrpc::uart::RequestError>
          response) noexcept {
    using SuccessCb = halstd::Callback<
        std::expected<std::reference_wrapper<const Res>, Error>>;
    if (!std::holds_alternative<const SuccessCb*>(inner_callback)) {
      return;
    }

    const auto callback = std::get<const SuccessCb*>(inner_callback);
    if (response.has_value()) {
      Res res{};
      if (vrpc::ProtoDecode(response.value(), res)) {
        if (callback != nullptr) {
          (*callback)(std::cref(res));
        }
      } else {
        (*callback)(std::unexpected(vrpc::uart::RequestError::DecodeFailed));
      }
    } else {
      (*callback)(std::unexpected(response.error()));
    }

    inner_callback = std::monostate{};

    // success_callback = std::monostate{};
    // error_callback   = nullptr;
  }
  //
  // void InvokeErrorCallback(vrpc::uart::RequestError error) const noexcept {
  //   if (error_callback != nullptr) {
  //     (*error_callback)(error);
  //   }
  // }

  ClientTransport<Uart, Sys, NC, O>& uart_client;

  halstd::DynamicMethodCallback<
      ServiceClientBase,
      std::expected<std::span<const std::byte>, RequestError>>
      req_cb;

  std::variant<std::monostate,
               const halstd::Callback<std::expected<
                   std::reference_wrapper<const ResponseMsgs>, Error>>*...>
      inner_callback{std::monostate{}};
  //
  // halstd::Callback<RequestError>* error_callback{nullptr};
};

}   // namespace vrpc::uart