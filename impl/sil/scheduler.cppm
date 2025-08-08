module;

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <format>
#include <latch>
#include <map>
#include <mutex>
#include <thread>

export module hal.sil:scheduler;

import hstd;

namespace sil {

export using TimePointUs = std::chrono::duration<uint64_t, std::micro>;
export using DurationUs  = std::chrono::duration<uint64_t, std::micro>;

export struct TimeoutExpired {};

export enum class SchedulerState {
  Stopped,
  Started,
  Stopping,
};

/**
 * Discrete event scheduler. Can simulate at microsecond granularity. Only one
 * task is allowed to run at a time
 */
export class Scheduler {
 public:
  [[nodiscard]] SchedulerState GetState() const noexcept {
    return state.load();
  }

  void Start() {
    // Create a startup latch with the amount of announced threads
    startup_latch = std::make_unique<std::latch>(announced_threads_count);

    // Unlock the system mutex such that the tasks can start initialization
    sys_lk.unlock();

    // Wait untill all threads have passed the startup latch. At this point,
    // the last task to initialize is guaranteed to have the system mutex
    startup_latch->wait();

    // Unlock the system mutex. This will return once all tasks are blocking
    // until the Epoch event
    sys_lk.lock();

    // Build the priority brackets
    for (auto& [_, thread] : threads) {
      auto bracket_it = std::ranges::find_if(
          priority_brackets, [&thread](const PriorityBracket& pb) {
            return pb.prio == thread.prio;
          });

      if (bracket_it == priority_brackets.end()) {
        priority_brackets.emplace_back(PriorityBracket{
            .prio = thread.prio, .rr_idx = 0, .threads = {&thread}});
      } else {
        bracket_it->threads.push_back(&thread);
      }
    }

    std::ranges::sort(priority_brackets,
                      [](const PriorityBracket& a, const PriorityBracket& b) {
                        return a.prio < b.prio;
                      });

    // Start the scheduler
    state.store(SchedulerState::Started);

    // Validate that the expected amount of threads was created
    if (threads.size() != announced_threads_count) {
      throw std::runtime_error{std::format(
          "Expected {} threads to have been created, but {} are initialized",
          threads.size(), announced_threads_count)};
    }

    // Unlock the system lock
    sys_lk.unlock();
  }

  /**
   * Simulates up to the given time point
   * @param time Time until which to simulate
   */
  void RunUntil(TimePointUs time) {
    // Ensure RunUntil is not called from one of the simulated task threads
    if (threads.contains(std::this_thread::get_id())) {
      throw std::runtime_error{
          "Scheduler::RunUntil() cannot be called from a simulated task "
          "thread."};
    }

    while (now.load() < time) {
      const auto thread_ran = HandleNextThread();

      if (!thread_ran) {
        const auto next_block_timeout = GetNextBlockTimeout();
        if (next_block_timeout.has_value()) {
          now.store(std::min(time, *next_block_timeout));
        } else {
          now.store(time);
          return;
        }
      }
    }
  }

  void Shutdown(std::size_t max_wakeups = 100) {
    // Mark scheduler as stopped
    state.store(SchedulerState::Stopping);

    // Keep on handling threads until all threads are shut down
    std::size_t wakeups = 0;
    while (!AllThreadsStopped()) {
      const auto thread_ran = HandleNextThread();

      if (thread_ran) {
        wakeups++;
      } else {
        const auto next_block_timeout = GetNextBlockTimeout();
        if (!next_block_timeout.has_value()) {
          throw std::runtime_error{
              "Error in Scheduler::Shutdown, no threads have a timeout set for "
              "unblocking, but not all threads are stopped"};
        } else {
          now.store(*next_block_timeout);
        }
      }

      if (wakeups > max_wakeups) {
        throw std::runtime_error{
            std::format("Error in Scheduler::Shutdown, not all threads are "
                        "stopped within {} wakeups.",
                        max_wakeups)};
      }
    }
  }

  /**
   * Blocks the current thread until the simulated point in time
   * @param time Simulated time point until which to block
   */
  void BlockCurrentThreadUntil(hstd::Duration auto time) {
    const auto time_us = std::chrono::duration_cast<DurationUs>(time);

    // Validate time point until which is blocked is not in the past
    if (now.load() > time_us) {
      throw std::runtime_error{
          std::format("Cannot block to time point {}, which is in the past "
                      "(current simulated time is {}).",
                      time, now.load())};
    }

    // Add a scheduler entry
    SetCurrentThreadBlockTimeout(time_us);

    // Wait until woken
    BlockCurrentThreadUntilWoken();
  }

  /**
   * Initializes the current thread. Must be called at the simulated epoch.
   */
  void InitializeThread() {
    if (now.load() != Epoch || state.load() != SchedulerState::Stopped) {
      throw std::runtime_error{std::format(
          "Scheduler::InitializeThread() can only be called when the scheduler "
          "is still at the simulated epoch and is not started, got {} and {}.",
          now.load(), std::to_underlying(state.load()))};
    }

    const auto tid = std::this_thread::get_id();
    if (threads.contains(tid)) {
      throw std::runtime_error{
          std::format("Thread {} was already initialized.", tid)};
    }

    threads.emplace(std::piecewise_construct, std::forward_as_tuple(tid),
                    std::forward_as_tuple(tid, sys_mtx));

    // Obtain a system mutex lock for the current thread
    auto& ts = GetCurrentThreadState();
    ts.lk.lock();

    // Add a scheduler entry at the epoch
    SetCurrentThreadBlockTimeout(Epoch);

    startup_latch->count_down();

    // Wait until woken at epoch
    BlockCurrentThreadUntilWoken();
  }

  /**
   * Announces the presence of an additional thread
   */
  void AnnounceThread() { announced_threads_count++; }

  /**
   * Announces that the current thread is done and can be joined
   */
  void DeInitializeThread() {
    auto& ts = GetCurrentThreadState();

    // Mark the current thread as not running
    ts.running = false;

    // Release the system mutex
    ts.lk.unlock();

    // Indicate current thread is "blocked", so that the scheduler thread
    // can continue
    running_thread_is_blocked.store(true);
    running_thread_is_blocked.notify_one();
  }

  /**
   * Returns the current simulated time of the scheduler
   * @return Current simulated time
   */
  TimePointUs Now() const noexcept { return now.load(); }

 private:
  struct ThreadState {
    ThreadState(std::thread::id id, std::mutex& mtx)
        : id{id}
        , cv{}
        , lk{mtx, std::defer_lock} {}

    unsigned prio;            //!< Thread priority
    bool     running{true};   //!< Whether the thread is running

    std::thread::id              id;   //!< Thread ID
    std::condition_variable      cv;   //!< Thread condition variable
    std::unique_lock<std::mutex> lk;   //!< Thread lock on system mutex

    std::optional<TimePointUs>
        block_timeout_at{};   //!< Timeout expiry time point
  };

  struct PriorityBracket {
    unsigned                 prio;
    std::size_t              rr_idx{0};
    std::deque<ThreadState*> threads;
  };

  static constexpr auto Epoch = TimePointUs{};

  /**
   * Returns the ThreadState associated with the thread in which the function
   * is called
   * @return Current thread state
   */
  ThreadState& GetCurrentThreadState() & {
    const auto tid = std::this_thread::get_id();
    if (!threads.contains(tid)) {
      throw std::runtime_error{std::format(
          "No ThreadState was constructed for this thread (id {}).", tid)};
    }

    return threads.at(tid);
  }

  /**
   * Sets the time point at which the current thread will unblock due to its
   * timeout expiring
   * @param timeout_at Timeout time point
   */
  void SetCurrentThreadBlockTimeout(TimePointUs timeout_at) {
    GetCurrentThreadState().block_timeout_at = timeout_at;
  }

  /**
   * Blocks the current thread until it is woken
   */
  void BlockCurrentThreadUntilWoken() {
    // Block the condition variable until the requested time point
    auto& ts = GetCurrentThreadState();
    running_thread_is_blocked.store(true);
    running_thread_is_blocked.notify_one();

    ts.cv.wait(ts.lk, [&ts, this] {
      const auto sched_state = state.load();
      const auto at_timeout =
          ts.block_timeout_at.has_value() && *ts.block_timeout_at == now.load();

      const auto cond = at_timeout
                        && (sched_state == SchedulerState::Started
                            || sched_state == SchedulerState::Stopping);
      return cond;
    });

    // Clear the block reasons
    ts.block_timeout_at = std::nullopt;
  }

  /**
   * Handles a single thread
   * @returns Whether a thread was woken
   */
  bool HandleNextThread() {
    bool thread_ran = false;
    for (auto& bracket : priority_brackets) {
      for (std::size_t i = 0; i < bracket.threads.size(); i++) {
        auto& thread =
            bracket.threads.at((i + bracket.rr_idx) % bracket.threads.size());

        if (!thread->running) {
          continue;
        }

        if (thread->block_timeout_at == now.load()) {
          // Mark the scheduler as running
          running_thread_is_blocked.store(false);

          // Wake up the thread
          thread->cv.notify_all();

          // Wait until the woken thread is blocked again
          running_thread_is_blocked.wait(false);

          // Restart the full iteration
          thread_ran = true;
          break;
        }
      }

      if (thread_ran) {
        bracket.rr_idx = (bracket.rr_idx + 1) % bracket.threads.size();
        return thread_ran;
      }
    }

    return thread_ran;
  }

  /**
   * Returns the earliest time point at which any thread will unblock due to
   * its block timeout expiring, or std::nullopt if no thread is currently
   * waiting on a timeout
   * @return Earliest time point at which any thread will unblock due to timeout
   */
  std::optional<TimePointUs> GetNextBlockTimeout() {
    std::optional<TimePointUs> next_timeout{std::nullopt};

    for (const auto& bracket : priority_brackets) {
      for (const auto& thread : bracket.threads) {
        if (thread->block_timeout_at
            && (next_timeout == std::nullopt
                || thread->block_timeout_at < next_timeout)) {
          next_timeout = thread->block_timeout_at;
        }
      }
    }

    return next_timeout;
  }

  /**
   * Returns whether all threads have stopped
   * @return Whether all threads have stopped
   */
  bool AllThreadsStopped() const {
    return std::ranges::all_of(
        threads, [](const auto& kvp) { return !kvp.second.running; });
  }

  std::size_t                 announced_threads_count{0};
  std::atomic<SchedulerState> state{SchedulerState::Stopped};
  std::atomic<TimePointUs>    now{Epoch};

  std::map<std::thread::id, ThreadState> threads{};
  std::deque<PriorityBracket>            priority_brackets{};
  std::mutex                             sys_mtx{};
  std::atomic<bool>                      running_thread_is_blocked{};
  std::atomic<unsigned>                  entry_id{0};

  std::unique_lock<std::mutex> sys_lk{sys_mtx};
  std::unique_ptr<std::latch>  startup_latch{nullptr};
};

}   // namespace sil