module;

#include <algorithm>
#include <format>
#include <iostream>
#include <latch>
#include <mutex>
#include <ranges>
#include <thread>

module hal.sil;

import hstd;

namespace sil {

SchedulerState Scheduler::GetState() const noexcept {
  return state.load();
}

void Scheduler::Start() {
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

  // All items are now registered, so priority brackets can be initialized
  InitializePriorityBrackets();

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

  // Simulate all events at the epoch
  RunUntil(Epoch, true);
}

void Scheduler::RunUntil(TimePointUs time, bool inclusive) {
  // Ensure RunUntil is not called from one of the simulated task threads
  if (threads.contains(std::this_thread::get_id())) {
    throw std::runtime_error{
        "Scheduler::RunUntil() cannot be called from a simulated task "
        "thread."};
  }

  // Keep on handling threads until the target time is reached
  while ((!inclusive && now.load() < time)
         || (inclusive && now.load() <= time)) {
    const auto thread_ran = HandleNextItem();

    if (!thread_ran) {
      const auto next_block_timeout_opt = GetNextBlockTimeout();
      if (next_block_timeout_opt.has_value()) {
        const auto next_block_timeout = *next_block_timeout_opt;

        if (next_block_timeout > time) {
          now.store(time);
          return;
        } else {
          now.store(next_block_timeout);
        }
      } else {
        now.store(time);
        return;
      }
    }
  }
}

bool Scheduler::RunUntilNextTimePoint(TimePointUs upper_bound) {
  // Ensure RunUntil is not called from one of the simulated task threads
  if (threads.contains(std::this_thread::get_id())) {
    throw std::runtime_error{
        "Scheduler::RunUntil() cannot be called from a simulated task "
        "thread."};
  }

  // Attempt to handle all entries for this time instance
  auto handled_item             = true;
  auto ran_item_at_current_time = false;

  while (handled_item) {
    handled_item = HandleNextItem();
    if (handled_item) {
      ran_item_at_current_time = true;
    }
  }

  // If any item ran at this time, return. Otherwise, advance time
  if (ran_item_at_current_time) {
    return true;
  }

  const auto next_block_timeout = GetNextBlockTimeout();
  if (next_block_timeout.has_value() && *next_block_timeout <= upper_bound) {
    now.store(*next_block_timeout);
  } else {
    now.store(upper_bound);
    return false;
  }

  // Handle any items at this time instance
  handled_item = true;
  while (handled_item) {
    handled_item = HandleNextItem();
  }

  return true;
}

void Scheduler::Shutdown(std::size_t max_wakeups) {
  // Mark scheduler as stopped
  state.store(SchedulerState::Stopping);

  // Keep on handling threads until all threads are shut down
  std::size_t wakeups = 0;
  while (!AllThreadsStopped()) {
    const auto thread_ran = HandleNextItem();

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

void Scheduler::CheckSyncPrimitivePreemption() {
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

ExternalEventItem& Scheduler::RegisterExternalEvent(unsigned prio) & {
  external_events.push_back(ExternalEventItem{prio});
  return external_events.back();
}

void Scheduler::AnnounceThread() {
  announced_threads_count++;
}

void Scheduler::InitializeThread(unsigned prio) {
  if (now.load() != Epoch || state.load() != SchedulerState::Stopped) {
    throw std::runtime_error{std::format(
        "Scheduler::InitializeThread() can only be called when the scheduler "
        "is still at the simulated epoch and is not started, got {} and {}.",
        now.load(), std::to_underlying(state.load()))};
  }

  // Lock the system mutex to ensure the startup latch has been created
  std::latch* startup_latch_ptr = nullptr;
  {
    std::scoped_lock lk{sys_mtx};

    const auto tid = std::this_thread::get_id();
    if (threads.contains(tid)) {
      throw std::runtime_error{
          std::format("Thread {} was already initialized.", tid)};
    }

    threads.emplace(std::piecewise_construct, std::forward_as_tuple(tid),
                    std::forward_as_tuple(tid, prio, sys_mtx));

    startup_latch_ptr = startup_latch.get();
  }

  GetCurrentThread().Initialize(*this, *startup_latch_ptr);
}

void Scheduler::DeInitializeThread() {
  GetCurrentThread().MarkStopped();

  // Indicate current thread is "blocked", so that the scheduler thread
  // can continue
  running_thread_is_blocked.store(true);
  running_thread_is_blocked.notify_all();
}

void Scheduler::MarkCurrentItemBlocked() {
  running_thread_is_blocked.store(true);
  running_thread_is_blocked.notify_all();
}

void Scheduler::InitializePriorityBrackets() {
  // Add threads to the priority brackets
  for (auto& thread : threads | std::views::values) {
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

  // Add the external events to the brackets
  for (auto& event : external_events) {
    auto bracket_it = std::ranges::find_if(
        priority_brackets, [&event](const PriorityBracket& pb) {
          return pb.prio == event.GetPriority();
        });

    if (bracket_it == priority_brackets.end()) {
      priority_brackets.emplace_back(PriorityBracket{
          .prio = event.GetPriority(), .rr_idx = 0, .items = {&event}});
    } else {
      bracket_it->items.push_back(&event);
    }
  }

  std::ranges::sort(priority_brackets,
                    [](const PriorityBracket& a, const PriorityBracket& b) {
                      return a.prio < b.prio;
                    });
}

const ThreadItem& Scheduler::GetCurrentThread() const& {
  const auto tid = std::this_thread::get_id();
  if (!threads.contains(tid)) {
    throw std::runtime_error{std::format(
        "No ThreadState was constructed for this thread (id {}).", tid)};
  }

  return threads.at(tid);
}

ThreadItem& Scheduler::GetCurrentThread() & {
  const auto tid = std::this_thread::get_id();
  if (!threads.contains(tid)) {
    throw std::runtime_error{std::format(
        "No ThreadState was constructed for this thread (id {}).", tid)};
  }

  return threads.at(tid);
}

void Scheduler::YieldCurrentThread() {
  GetCurrentThread().Yield(*this);
}

bool Scheduler::HandleNextItem() {
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
        const auto run_type = item->Run();

        // If the action is running asynchronously, block until the woken thread
        // is blocked again
        if (run_type == RunType::Asynchronous) {
          running_thread_is_blocked.wait(false);
        } else {
          running_thread_is_blocked.store(true);
        }

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

std::optional<TimePointUs>
Scheduler::GetNextBlockTimeout(GetBlockTimeoutOpts opts) const {
  std::optional<TimePointUs> next_timeout{std::nullopt};

  ItemPrio min_prio = opts.min_prio;

  if (opts.higher_than_current_prio) {
    const auto current_thread_prio = GetCurrentThread().GetPriority();
    if (current_thread_prio < opts.min_prio) {
      min_prio = current_thread_prio;
    }
  }

  for (const auto& bracket : priority_brackets) {
    for (const auto& item : bracket.items) {
      if (const auto to = item->GetTimeout();
          to && (next_timeout == std::nullopt || to < next_timeout)) {
        // Handle "exclude_now"
        if (opts.exclude_now && *to > now.load()) {
          continue;
        }

        // Handle minimum priority
        if (*to == now.load() && item->GetPriority() > min_prio) {
          continue;
        }

        next_timeout = to;
      }
    }
  }

  return next_timeout;
}

bool Scheduler::AllThreadsStopped() const {
  return std::ranges::all_of(
      threads, [](const auto& kvp) { return !kvp.second.IsRunning(); });
}

}   // namespace sil
