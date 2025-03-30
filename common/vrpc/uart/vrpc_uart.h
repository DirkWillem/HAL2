#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <ranges>
#include <span>

#include <hal/system.h>
#include <hal/uart.h>

#include <constexpr_tools/helpers.h>
#include <constexpr_tools/math.h>

#include <vrpc/generated/common/vrpc.h>
#include <vrpc/proto_helpers.h>
#include <vrpc/server.h>

#include "vrpc_uart_decoder.h"
#include "vrpc_uart_encode.h"

namespace vrpc::uart {
enum class HandleState {
  Handled,
  HandlingAsync,
  ErrUnknownCommand,
  ErrMalformedPayload,
  ErrEncodeFailure,
};

struct HandleResult {
  HandleState                state;
  std::span<const std::byte> response_payload;
};

template <typename Impl>
concept ServiceImpl = requires(Impl& impl) {
  { Impl::ServiceId } -> std::convertible_to<uint32_t>;

  requires ct::IsConstantExpression<Impl::MinBufferSize()>();
  { Impl::MinBufferSize() } -> std::convertible_to<std::size_t>;

  {
    impl.HandleCommand(std::declval<uint32_t>(),
                       std::declval<std::span<const std::byte>>(),
                       std::declval<std::span<std::byte>>(),
                       std::declval<halstd::Callback<HandleResult>&>())
  } -> std::convertible_to<HandleResult>;
};

namespace detail {

template <ServiceImpl Service>
class ServiceRef {
 protected:
  explicit ServiceRef(Service& service) noexcept
      : service{service} {}

  Service& service;
};

}   // namespace detail

template <typename Service, template <typename> typename UartImpl>
struct UartService {
  template <typename... Args>
  explicit UartService(Args&&... args)
      : svc{std::forward<Args>(args)...}
      , svc_uart{svc} {}

  Service           svc;
  UartImpl<Service> svc_uart;
};

template <bool Enable, typename Response>
class AsyncCommandCallback;

template <typename Response>
class AsyncCommandCallback<false, Response> {};

template <typename Response>
class AsyncCommandCallback<true, Response> {
public:
  AsyncResult<Response> InitializeCallback(
    std::span<std::byte>            new_response_buf,
    halstd::Callback<HandleResult>& new_inner_callback) & noexcept {
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

  halstd::MethodCallback<AsyncCommandCallback<true, Response>> callback;
  halstd::Callback<HandleResult>* inner_callback{nullptr};
};

}   // namespace vrpc::uart