module;

#include <latch>
#include <mutex>
#include <optional>
#include <thread>

module hal.sil;

namespace sil {

ThreadItem::ThreadItem(std::thread::id id, unsigned prio, std::mutex& mtx)
    : SchedulerItem{}
    , id{id}
    , prio{prio}
    , cv{}
    , lk{mtx, std::defer_lock} {}

void ThreadItem::Initialize(Scheduler& sched, std::latch& startup_latch) {
  lk.lock();
  block_timeout_at = TimePointUs{0};

  startup_latch.count_down();

  BlockUntilWoken(sched);
}

void ThreadItem::MarkStopped() {
  running = false;
  lk.unlock();
}

ItemPrio ThreadItem::GetPriority() const {
  return {ThreadPriorityLevel, prio};
}

bool ThreadItem::IsRunning() const {
  return running;
}

bool ThreadItem::IsPending(TimePointUs time) const {
  if (block_timeout_at.has_value() && *block_timeout_at == time) {
    return true;
  }

  if (sync_primitive_block.has_value()
      && sync_primitive_block->check_unblock()) {
    return true;
  }

  return false;
}

std::optional<TimePointUs> ThreadItem::GetTimeout() const {
  return block_timeout_at;
}

RunType ThreadItem::Run() {
  wakeup_requested.store(true);
  cv.notify_one();
  return RunType::Asynchronous;
}

void ThreadItem::BlockUntilUs(DurationUs time, Scheduler& sched) {
  block_timeout_at = time;
  BlockUntilWoken(sched);
}

void ThreadItem::Yield(Scheduler& sched) {
  block_timeout_at = sched.Now();
  BlockUntilWoken(sched);
}

ThreadItem::UnblockReason ThreadItem::BlockUntilWoken(Scheduler& sched) {
  sched.MarkCurrentItemBlocked();

  cv.wait(lk, [this] { return wakeup_requested.exchange(false); });

  // Determine unblock reason
  auto reason = UnblockReason::Timeout;

  if (sync_primitive_block.has_value()
      && sync_primitive_block->check_unblock()) {
    reason = UnblockReason::SyncPrimitive;
  }

  // Clear all unblock reasons
  block_timeout_at     = std::nullopt;
  sync_primitive_block = std::nullopt;

  return reason;
}

}   // namespace sil