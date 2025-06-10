module;

#include <chrono>
#include <cstdint>
#include <span>
#include <variant>

export module vrpc.uart.rtos.server:service_task;

import hstd;

import rtos.freertos;

import vrpc.uart.common;

namespace vrpc::uart {

export template <std::size_t StackSize, typename Impl, typename RequestMsgs,
                 typename ResponseMsgs>
class ServiceTask {};

export template <std::size_t StackSize, typename Impl, typename... RequestMsgs,
                 typename... ResponseMsgs>
class ServiceTask<StackSize, Impl, hstd::Types<RequestMsgs...>,
                  hstd::Types<ResponseMsgs...>>
    : public rtos::Task<
          ServiceTask<StackSize, Impl, hstd::Types<RequestMsgs...>,
                      hstd::Types<ResponseMsgs...>>,
          StackSize> {
  struct PendingRequest {
    uint32_t                   command_id;
    std::span<const std::byte> request_buf;
    std::span<std::byte>       response_buf;
    HandleResult*              handle_result;
    rtos::EventGroup*          response_event_group;
    uint32_t                   response_event_bitmask;
  };

 public:
  bool Request(uint32_t request_id, std::span<const std::byte> request_buf,
               std::span<std::byte>& response_buf,
               rtos::EventGroup&     response_event_group,
               uint32_t              response_event_bitmask,
               HandleResult&         handle_result) noexcept {
    return request_queue.TryEnqueue(PendingRequest{
        .command_id             = request_id,
        .request_buf            = request_buf,
        .response_buf           = response_buf,
        .handle_result          = &handle_result,
        .response_event_group   = &response_event_group,
        .response_event_bitmask = response_event_bitmask,
    });
  }

 protected:
  ServiceTask(const char* name)
      : rtos::Task<ServiceTask<StackSize, Impl, hstd::Types<RequestMsgs...>,
                               hstd::Types<ResponseMsgs...>>>{name} {}

  [[noreturn]] void operator()() {
    using namespace std::chrono_literals;

    while (true) {
      if (auto pending_request_opt = request_queue.Dequeue(1000ms);
          pending_request_opt.has_value()) {
        auto pending_request = *pending_request_opt;
      }
    }
  }

 private:
  static constexpr auto RequestAvailableBit = (0b1U << 0U);

  rtos::EventGroup               event_group;
  rtos::Queue<PendingRequest, 1> request_queue;
};

}   // namespace vrpc::uart
