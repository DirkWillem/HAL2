#pragma once

#include <algorithm>

#include <constexpr_tools/logic.h>

#include <vrpc/proto_helpers.h>
#include <vrpc/uart/vrpc_uart.h>

#include <hal/callback.h>

#include <calculator_service.h>

template <typename Impl>
concept CalculatorServiceCalculateSync = requires(Impl& impl) {
  impl.Calculate(std::declval<const calculator::CalculatorRequest&>(),
                 std::declval<calculator::CalculatorResponse&>());
};

template <typename Impl>
concept CalculatorServiceCalculateAsync = requires(Impl& impl) {
  impl.Calculate(
      std::declval<const calculator::CalculatorRequest&>(),
      std::declval<hal::Callback<const calculator::CalculatorResponse&>&>());
};

// template <typename Impl>
// concept CalculatorService = (ct::Xor(CalculatorServiceCalculateSync<Impl>,
//                                      CalculatorServiceCalculateAsync<Impl>));

template <typename Impl>
concept CalculatorService = CalculatorServiceCalculateSync<Impl>;

template <CalculatorService Impl>
class UartCalculatorService {
 public:
  static constexpr auto ServiceId = calculator::CalculatorServiceId;

  static consteval std::size_t MinBufferSize() {
    return std::max({
        nanopb::MessageDescriptor<calculator::CalculatorRequest>::size,
        nanopb::MessageDescriptor<calculator::CalculatorResponse>::size,
    });
  }

  explicit UartCalculatorService(Impl& impl)
      : impl{impl} {}

  vrpc::uart::HandleResult
  HandleCommand(uint32_t id, std::span<const std::byte> request_buf,
                std::span<std::byte> response_buf,
                hal::Callback<>&     async_complete_callback) {
    switch (id) {
    case 0x10: {
      // Decode request
      calculator::CalculatorRequest request{};
      if (!vrpc::ProtoDecode(request_buf, request)) {
        return {
            .state            = vrpc::uart::HandleState::ErrMalformedPayload,
            .response_payload = {},
        };
      }

      // Handle command
      if constexpr (CalculatorServiceCalculateSync<Impl>) {
        calculator::CalculatorResponse response{};
        impl.Calculate(request, response);

        const auto [enc_success, enc_data] =
            vrpc::ProtoEncode(response, response_buf);

        if (!enc_success) {
          return {
              .state            = vrpc::uart::HandleState::ErrEncodeFailure,
              .response_payload = {},
          };
        }

        return {
            .state            = vrpc::uart::HandleState::Handled,
            .response_payload = enc_data,
        };
      } else {
      }
    }
    default:
      return {
          .state            = vrpc::uart::HandleState::ErrUnknownCommand,
          .response_payload = {},
      };
    }
  }

  Impl& impl;
};

class CalculatorServiceImpl {
 public:
  constexpr void Calculate(const calculator::CalculatorRequest& req,
                           calculator::CalculatorResponse&      res) {
    switch (static_cast<calculator::CalculatorOp>(req.op)) {
    case calculator::CalculatorOp::Add: res.result = req.lhs + req.rhs; break;
    case calculator::CalculatorOp::Sub: res.result = req.lhs - req.rhs; break;
    case calculator::CalculatorOp::Mul: res.result = req.lhs * req.rhs; break;
    case calculator::CalculatorOp::Div:
      if (req.rhs != 0) {
        res.result = req.lhs / req.rhs;
      }
      break;
    }
  }
};

static_assert(CalculatorService<CalculatorServiceImpl>);
