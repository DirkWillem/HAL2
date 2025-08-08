module;

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <format>
#include <functional>
#include <latch>
#include <map>
#include <mutex>
#include <thread>
#include <tuple>
#include <variant>

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

inline constexpr unsigned ThreadPriorityLevel = 1;

export class Scheduler;

using ItemPrio = std::tuple<unsigned, unsigned>;

class SchedulerItem {
 public:
  SchedulerItem()          = default;
  virtual ~SchedulerItem() = default;

  /**
   * Returns the item priority
   * @return Item priority
   */
  [[nodiscard]] virtual ItemPrio GetPriority() const = 0;

  /**
   * Returns whether the item is still active
   * @return Whether the item is still active
   */
  [[nodiscard]] virtual bool IsRunning() const = 0;

  /**
   * Returns whether the item is pending execution
   * @param time Current time
   * @return Whether the item is pending
   */
  [[nodiscard]] virtual bool IsPending(TimePointUs time) const = 0;

  /**
   * Returns the timeout of the item
   * @return Current timeout of the item, or nullopt if it has none
   */
  [[nodiscard]] virtual std::optional<TimePointUs> GetTimeout() const = 0;

  /**
   * Runs the scheduler item until it enters a blocking state
   */
  virtual void Run() = 0;
};

class ThreadItem final : public SchedulerItem {
  struct SyncPrimitiveBlock {
    void*                 ptr;
    std::function<bool()> check_unblock;
  };

  enum class UnblockReason {
    Timeout,
    SyncPrimitive,
  };

 public:
  /**
   * Constructor
   * @param id Thread ID
   * @param prio Thread priority
   * @param mtx System mutex
   */
  ThreadItem(std::thread::id id, unsigned prio, std::mutex& mtx);

  ~ThreadItem() final = default;

  ItemPrio GetPriority() const final;

  bool                       IsRunning() const final;
  bool                       IsPending(TimePointUs time) const final;
  std::optional<TimePointUs> GetTimeout() const final;

  void Run() final;

  /**
   * Initializes the thread. This sets its timeout to the epoch and acquires
   * an initial lock on the system mutex
   * @param sched Scheduler reference
   * @param startup_latch Latch for keeping track of initialization
   */
  void Initialize(Scheduler& sched, std::latch& startup_latch);

  /**
   * Marks the thread as stopped and releases the lock on the system mutex for
   * good
   */
  void MarkStopped();

  /**
   * Blocks the thread until the given time point
   * @param time Time point until which to block
   * @param sched Scheduler reference
   */
  void BlockUntil(hstd::Duration auto time, Scheduler& sched) {
    BlockUntilUs(std::chrono::duration_cast<DurationUs>(time), sched);
  }

  template <std::invocable T>
    requires hstd::concepts::Optional<std::decay_t<std::invoke_result_t<T>>>
  std::variant<TimeoutExpired, typename std::invoke_result_t<T>::value_type>
  BlockOnSyncPrimitive(void* primitive, const T& check_unblock,
                       hstd::Duration auto timeout, Scheduler& sched) {
    // Set timeout and sync primitive block
    const auto time_us   = std::chrono::duration_cast<DurationUs>(timeout);
    block_timeout_at     = time_us;
    sync_primitive_block = {.ptr           = primitive,
                            .check_unblock = [check_unblock]() {
                              return check_unblock().has_value();
                            }};

    // Wait until woken
    const auto reason = BlockUntilWoken(sched);

    // Return unblock reason
    switch (reason) {
    case UnblockReason::Timeout: return TimeoutExpired{};
    case UnblockReason::SyncPrimitive: return *check_unblock();
    }

    throw std::runtime_error{"Invalid unblock reason"};
  }

  /**
   * Yields the thread
   * @param sched Scheduler reference
   */
  void Yield(Scheduler& sched);

 private:
  void          BlockUntilUs(DurationUs time, Scheduler& sched);
  UnblockReason BlockUntilWoken(Scheduler& sched);

  std::thread::id   id;              //!< Thread ID
  unsigned          prio;            //!< Thread priority
  bool              running{true};   //!< Whether the thread is running
  std::atomic<bool> wakeup_requested{
      false};   //!< Whether a wakeup of the thread is requested

  std::condition_variable      cv;   //!< Thread condition variable
  std::unique_lock<std::mutex> lk;   //!< Thread lock on system mutex

  std::optional<TimePointUs>
      block_timeout_at{};   //!< Timeout expiry time point
  std::optional<SyncPrimitiveBlock>
      sync_primitive_block{};   //!< Blocking condition due to synchronization
  //!< primitive
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
            return pb.prio == thread.GetPriority();
          });

      if (bracket_it == priority_brackets.end()) {
        priority_brackets.emplace_back(PriorityBracket{
            .prio = thread.GetPriority(), .rr_idx = 0, .items = {&thread}});
      } else {
        bracket_it->items.push_back(&thread);
      }
    }

    std::ranges::sort(priority_brackets,
                      [](const PriorityBracket& a, const PriorityBracket& b) {
                        return a.prio > b.prio;
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
    GetCurrentThread().BlockUntil(time, *this);
  }

  template <std::invocable T>
    requires hstd::concepts::Optional<std::decay_t<std::invoke_result_t<T>>>
  auto BlockCurrentThreadOnSynchronizationPrimitive(
      void* primitive, const T& check_unblock, hstd::Duration auto timeout) {
    return GetCurrentThread().BlockOnSyncPrimitive(primitive, check_unblock,
                                                   timeout, *this);
  }

  /**
   * Should be called when a thread changes a synchronization primitive. Allows
   * the current thread to be preempted by higher-priority threads
   */
  void CheckSyncPrimitivePreemption() {
    const auto preempted_prio = GetCurrentThread().GetPriority();

    for (const auto& bracket : priority_brackets) {
      if (bracket.prio <= preempted_prio) {
        return;
      }

      for (const auto& thread : bracket.items) {
        if (thread->IsPending(Now())) {
          YieldCurrentThread();
          return;
        }
      }
    }
  }

  /**
   * Initializes the current thread. Must be called at the simulated epoch.
   */
  void InitializeThread(unsigned prio) {
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
                    std::forward_as_tuple(tid, prio, sys_mtx));

    // Initialize the thread and block until simulation started
    GetCurrentThread().Initialize(*this, *startup_latch);
  }

  /**
   * Announces the presence of an additional thread
   */
  void AnnounceThread() { announced_threads_count++; }

  /**
   * Announces that the current thread is done and can be joined
   */
  void DeInitializeThread() {
    GetCurrentThread().MarkStopped();

    // Indicate current thread is "blocked", so that the scheduler thread
    // can continue
    running_thread_is_blocked.store(true);
    running_thread_is_blocked.notify_one();
  }

  /**
   * Marks the current action as having entered a blocking state
   */
  void MarkCurrentActionBlocked() {
    running_thread_is_blocked.store(true);
    running_thread_is_blocked.notify_one();
  }

  /**
   * Returns the current simulated time of the scheduler
   * @return Current simulated time
   */
  TimePointUs Now() const noexcept { return now.load(); }

 private:
  struct PriorityBracket {
    ItemPrio                   prio;
    std::size_t                rr_idx{0};
    std::deque<SchedulerItem*> items;
  };

  static constexpr auto Epoch = TimePointUs{};

  /**
   * Returns the ThreadState associated with the thread in which the function
   * is called
   * @return Current thread state
   */
  ThreadItem& GetCurrentThread() & {
    const auto tid = std::this_thread::get_id();
    if (!threads.contains(tid)) {
      throw std::runtime_error{std::format(
          "No ThreadState was constructed for this thread (id {}).", tid)};
    }

    return threads.at(tid);
  }

  /**
   * Yields the current thread, allowing higher-priority threads to run
   */
  void YieldCurrentThread() { GetCurrentThread().Yield(*this); }

  /**
   * Handles a single thread
   * @returns Whether a thread was woken
   */
  bool HandleNextThread() {
    bool thread_ran = false;
    for (auto& bracket : priority_brackets) {
      for (std::size_t i = 0; i < bracket.items.size(); i++) {
        auto& item =
            bracket.items.at((i + bracket.rr_idx) % bracket.items.size());

        if (!item->IsRunning()) {
          continue;
        }

        if (item->IsPending(now.load())) {
          // Mark the scheduler as running
          running_thread_is_blocked.store(false);

          // Wake up the thread
          item->Run();

          // Wait until the woken thread is blocked again
          running_thread_is_blocked.wait(false);

          // Restart the full iteration
          thread_ran = true;
          break;
        }
      }

      if (thread_ran) {
        bracket.rr_idx = (bracket.rr_idx + 1) % bracket.items.size();
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
      for (const auto& thread : bracket.items) {
        const auto to = thread->GetTimeout();
        if (to && (next_timeout == std::nullopt || to < next_timeout)) {
          next_timeout = to;
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
        threads, [](const auto& kvp) { return !kvp.second.IsRunning(); });
  }

  std::size_t                 announced_threads_count{0};
  std::atomic<SchedulerState> state{SchedulerState::Stopped};
  std::atomic<TimePointUs>    now{Epoch};

  std::map<std::thread::id, ThreadItem> threads{};
  std::deque<PriorityBracket>           priority_brackets{};
  std::mutex                            sys_mtx{};
  std::atomic<bool>                     running_thread_is_blocked{};
  std::atomic<unsigned>                 entry_id{0};

  std::unique_lock<std::mutex> sys_lk{sys_mtx};
  std::unique_ptr<std::latch>  startup_latch{nullptr};
};

}   // namespace sil