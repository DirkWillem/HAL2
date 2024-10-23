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
    impl.HandleCommand(
        std::declval<uint32_t>(), std::declval<std::span<const std::byte>>(),
        std::declval<std::span<std::byte>>(), std::declval<hal::Callback<>&>())
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

}   // namespace vrpc::uart