module;

#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

export module rtos.sil;

import hstd;

import rtos.concepts;

import hal.sil;

namespace rtos::sil {

template <typename Impl, std::size_t StackSize = 0>
class Task {
 public:
  explicit Task(std::string_view name)
      : name{name}
      , thread{[this]() {
        auto& sched = ::sil::System::instance().GetScheduler();
        sched.InitializeThread();
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

  template <typename Impl, std::size_t StackSize = 0>
  using Task = Task<Impl, StackSize>;

  using System = System;
};

static_assert(rtos::concepts::Rtos<Rtos>);

}   // namespace rtos::sil