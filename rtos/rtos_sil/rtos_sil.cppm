module;

#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <variant>

export module rtos.sil;

import hstd;

import rtos.concepts;

import hal.sil;

namespace rtos::sil {

struct EventGroupState {
  std::atomic<uint32_t> bits;
};

class EventGroup {
 public:
  EventGroup()
      : state{std::make_unique<EventGroupState>()} {}

  EventGroup(const EventGroup&)            = default;
  EventGroup(EventGroup&& rhs)             = default;
  EventGroup& operator=(const EventGroup&) = default;
  EventGroup& operator=(EventGroup&& rhs)  = default;
  ~EventGroup()                            = default;

  void SetBits(uint32_t bits) {
    state->bits.fetch_or(bits);
    sched().CheckSyncPrimitivePreemption();
  }

  void ClearBits(uint32_t bits) {
    state->bits.fetch_and(~bits);
    sched().CheckSyncPrimitivePreemption();
  }

  uint32_t ReadBits() const { return state->bits.load(); }

  std::optional<uint32_t> Wait(uint32_t            bits_to_wait,
                               hstd::Duration auto timeout,
                               bool                clear_on_exit = true,
                               bool                wait_for_all  = false) {
    const auto unblock_reason =
        sched().BlockCurrentThreadOnSynchronizationPrimitive(
            state.get(),
            [this, bits_to_wait, wait_for_all]() -> std::optional<uint32_t> {
              const auto bits     = ReadBits();
              const auto set_bits = bits & bits_to_wait;

              if (wait_for_all && set_bits == bits_to_wait) {
                return bits;
              } else if (!wait_for_all && set_bits != 0) {
                return bits;
              } else {
                return {};
              }
            },
            timeout);

    if (std::holds_alternative<::sil::TimeoutExpired>(unblock_reason)) {
      return std::nullopt;
    } else if (const auto bits = std::get_if<uint32_t>(&unblock_reason);
               bits != nullptr) {
      if (clear_on_exit) {
        state->bits.fetch_and(~bits_to_wait);
      }

      return *bits;
    }

    std::unreachable();
  }

 private:
  static ::sil::Scheduler& sched() {
    return ::sil::System::instance().GetScheduler();
  }

  std::shared_ptr<EventGroupState> state;
};

template <typename Impl, std::size_t StackSize = 0>
class Task {
 public:
  explicit Task(std::string_view name, unsigned priority = 0)
      : name{name}
      , thread{[this, priority]() {
        auto& sched = ::sil::System::instance().GetScheduler();
        sched.InitializeThread(priority);
        static_cast<Impl*>(this)->operator()();
        sched.DeInitializeThread();
      }} {
    ::sil::System::instance().GetScheduler().AnnounceThread();
  }

  ~Task() { thread.join(); }

  /**
   * Returns whether a stop of the task was requested
   * @return Whether a stop of the task was requested
   */
  [[nodiscard]] bool StopRequested() const {
    return ::sil::System::instance().GetScheduler().GetState()
           == ::sil::SchedulerState::Stopping;
  }

 private:
  std::string name;
  std::thread thread;
};

class OsClock {
 public:
  using rep        = uint32_t;
  using period     = std::milli;
  using duration   = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<OsClock, duration>;

  static constexpr auto is_steady = false;

  [[nodiscard]] static time_point now() {
    constexpr auto ClockEpoch = time_point{};
    const auto     now        = ClockEpoch + sched().Now();

    return std::chrono::time_point_cast<duration, OsClock>(
        std::chrono::time_point_cast<
            std::chrono::duration<uint64_t, std::milli>, OsClock>(now));
  }

  static void BlockFor(hstd::Duration auto duration) noexcept {
    sched().BlockCurrentThreadUntil(sched().Now() + duration);
  }

 private:
  static ::sil::Scheduler& sched() {
    return ::sil::System::instance().GetScheduler();
  }
};

export class NopCriticalSectionInterface {
 public:
  static void Enter() noexcept {}
  static void Exit() noexcept {}
};

export struct System {
  using CriticalSectionInterface = NopCriticalSectionInterface;
  using Clock                    = OsClock;

  template <typename T>
  using Atomic = std::atomic<T>;

  using AtomicFlag = std::atomic_flag;
};

export struct Rtos {
  static constexpr auto MiniStackSize       = 0;
  static constexpr auto SmallStackSize      = 0;
  static constexpr auto MediumStackSize     = 0;
  static constexpr auto LargeStackSize      = 0;
  static constexpr auto ExtraLargeStackSize = 0;

  using EventGroup = EventGroup;

  template <typename Impl, std::size_t StackSize = 0>
  using Task = Task<Impl, StackSize>;

  using System = System;
};

static_assert(rtos::concepts::Rtos<Rtos>);

}   // namespace rtos::sil