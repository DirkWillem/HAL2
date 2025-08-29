module;

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
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

enum class RunType {
  Synchronous,
  Asynchronous,
};

inline constexpr unsigned HighestPriorityLevel       = 0;
inline constexpr unsigned ExternalEventPriorityLevel = 1;
inline constexpr unsigned ThreadPriorityLevel        = 10;
inline constexpr unsigned LowestPriorityLevel        = 10'000;

export class Scheduler;

using ItemPrio = std::tuple<unsigned, unsigned>;

inline constexpr ItemPrio HighestPrio = {HighestPriorityLevel, 0};
inline constexpr ItemPrio LowestPrio  = {LowestPriorityLevel, 0};

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
  virtual RunType Run() = 0;
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

  RunType Run() final;

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

class ExternalEventItem final : public SchedulerItem {
 public:
  explicit ExternalEventItem(unsigned prio = 0);

  void RegisterPendingAction(TimePointUs           timestamp,
                             std::function<void()> callback);

  [[nodiscard]] bool HasPendingAction() const;

  [[nodiscard]] ItemPrio GetPriority() const final;
  [[nodiscard]] bool     IsRunning() const final;
  [[nodiscard]] bool     IsPending(TimePointUs time) const final;
  [[nodiscard]] std::optional<TimePointUs> GetTimeout() const final;

  RunType Run() final;

 private:
  struct Action {
    std::function<void()> callback;
    TimePointUs           timestamp;
  };

  std::optional<Action> pending_action{};   //!< Pending external action
  unsigned              priority{0};        //!< External action priority
};

/**
 * Options for finding the next block timeout
 */
export struct GetBlockTimeoutOpts {
  bool exclude_now = false;   //!< Whether to exclude now
  bool higher_than_current_prio =
      false;   //!< Whether to only include items that have higher priority
  //!< than the current thread
  ItemPrio min_prio = LowestPrio;
};

/**
 * Discrete event scheduler. Can simulate at microsecond granularity. Only one
 * task is allowed to run at a time
 */
export class Scheduler {
 public:
  [[nodiscard]] SchedulerState GetState() const noexcept;

  void Start();

  /**
   * Simulates up to the given time point
   * @param time Time until which to simulate
   * @param inclusive Whether to also simulate all events at the time limit
   */
  void RunUntil(TimePointUs time, bool inclusive = false);

  /**
   * Simulates up until the next time point. Behavior is dependent on whether
   * all events for the current time point have been processed:
   * - If the current time instance is done simulating, increases time to the
   *    next time point and simulates all events for that time point.
   * - If the current time instance is not done simulating, handles all events
   *    for this time point and does not advance time.
   *
   * @param upper_bound Max time until which the scheduler is allowed to
   *   simulate.
   * @returns Whether any item was handled.
   */
  bool RunUntilNextTimePoint(TimePointUs upper_bound);

  /**
   * Shuts down the scheduler and lets all threads finish
   * @param max_wakeups Maximum amount of wakeups that may occur before all
   * threads are shut down
   */
  void Shutdown(std::size_t max_wakeups = 100);

  /**
   * Blocks the current thread for the given amount of time
   * @param time Time to block the current thread for
   */
  void BlockCurrentThreadFor(hstd::Duration auto time) {
    GetCurrentThread().BlockUntil(now.load() + time, *this);
  }

  bool BlockCurrentThreadUntilNextTimepoint(hstd::Duration auto upper_bound) {
    const auto next_timeout_at_opt = GetNextBlockTimeout({
        .higher_than_current_prio = true,
    });

    if (next_timeout_at_opt.has_value()) {
      auto next_timeout_at = *next_timeout_at_opt;

      if (next_timeout_at > upper_bound) {
        BlockCurrentThreadUntil(upper_bound);
        return false;
      } else {
        BlockCurrentThreadUntil(next_timeout_at);
        return true;
      }
    } else {
      BlockCurrentThreadUntil(upper_bound);
      return false;
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
  void CheckSyncPrimitivePreemption();

  /**
   * Registers an external event with the scheduler
   * @param prio Priority of the external event
   * @return Created external event item
   */
  ExternalEventItem& RegisterExternalEvent(unsigned prio = 0) &;

  /**
   * Announces the presence of an additional thread
   */
  void AnnounceThread();

  /**
   * Initializes the current thread. Must be called at the simulated epoch.
   */
  void InitializeThread(unsigned prio);

  /**
   * Announces that the current thread is done and can be joined
   */
  void DeInitializeThread();

  /**
   * Marks the current action as having entered a blocking state
   */
  void MarkCurrentItemBlocked();

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

  void InitializePriorityBrackets();

  /**
   * Returns the ThreadState associated with the thread in which the function
   * is called
   * @return Current thread state
   */
  ThreadItem& GetCurrentThread() &;

  /**
   * Returns the ThreadState associated with the thread in which the function
   * is called
   * @return Current thread state
   */
  const ThreadItem& GetCurrentThread() const&;

  /**
   * Yields the current thread, allowing higher-priority threads to run
   */
  void YieldCurrentThread();

  /**
   * Handles a single scheduler item
   * @returns Whether an item was run
   */
  bool HandleNextItem();

  /**
   * Returns the earliest time point at which any thread will unblock due to
   * its block timeout expiring, or std::nullopt if no thread is currently
   * waiting on a timeout
   * @param opts Options to use when searching
   * @return Earliest time point at which any thread will unblock due to timeout
   */
  std::optional<TimePointUs>
  GetNextBlockTimeout(GetBlockTimeoutOpts opts = {}) const;

  /**
   * Returns whether all threads have stopped
   * @return Whether all threads have stopped
   */
  bool AllThreadsStopped() const;

  std::size_t                 announced_threads_count{0};
  std::atomic<SchedulerState> state{SchedulerState::Stopped};
  std::atomic<TimePointUs>    now{Epoch};

  std::map<std::thread::id, ThreadItem> threads{};
  std::deque<ExternalEventItem>         external_events{};
  std::deque<PriorityBracket>           priority_brackets{};
  std::mutex                            sys_mtx{};
  std::atomic<bool>                     running_thread_is_blocked{};
  std::atomic<unsigned>                 entry_id{0};

  std::unique_lock<std::mutex> sys_lk{sys_mtx};
  std::unique_ptr<std::latch>  startup_latch{nullptr};
};

}   // namespace sil