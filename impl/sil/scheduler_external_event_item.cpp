module;

#include <chrono>
#include <functional>
#include <tuple>

module hal.sil;

namespace sil {

ExternalEventItem::ExternalEventItem(unsigned prio)
    : priority{prio} {}

void ExternalEventItem::RegisterPendingAction(TimePointUs           timestamp,
                                              std::function<void()> callback) {
  if (pending_action.has_value()) {
    throw std::runtime_error{
        "Cannot register pending action on external event when the previously "
        "registered event hasn't been handled yet"};
  }

  pending_action = {
      .callback  = callback,
      .timestamp = timestamp,
  };
}

bool ExternalEventItem::HasPendingAction() const {
  return pending_action.has_value();
}

ItemPrio ExternalEventItem::GetPriority() const {
  return {ExternalEventPriorityLevel, priority};
}

bool ExternalEventItem::IsRunning() const {
  return HasPendingAction();
}

bool ExternalEventItem::IsPending(TimePointUs time) const {
  if (pending_action.has_value()) {
    return pending_action->timestamp == time;
  }

  return false;
}

std::optional<TimePointUs> ExternalEventItem::GetTimeout() const {
  return pending_action.transform([](const Action& a) { return a.timestamp; });
}

RunType ExternalEventItem::Run() {
  if (!pending_action) {
    throw std::runtime_error{
        "Cannot run ExternalEventItem when it has no pending action"};
  }

  pending_action->callback();
  pending_action.reset();
  return RunType::Synchronous;
}

}   // namespace sil