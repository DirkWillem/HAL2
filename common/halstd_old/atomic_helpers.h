#pragma once

#include <concepts>
#include <optional>

#include "atomic.h"

namespace halstd {

/**
 * Helper class that clears an atomic flag when it goes out of scope
 * @tparam F Atomic flag type
 */
template <AtomicFlag F>
class ClearFlagAtExit {
 public:
  explicit ClearFlagAtExit(F& flag)
      : flag{flag} {}

  ~ClearFlagAtExit() { flag.clear(); }

 private:
  F& flag;
};

/**
 * Performs an action based on exclusive access which is guarded using an atomic
 * flag
 * @tparam F Atomic flag type
 * @tparam A Action type
 * @param flag Reference to the atomic flag to use
 * @param action Action to perform
 * @return Action result as a std::optional, or std::nullopt when the action
 * could not be performed. In case of a void action, true when the action was
 * performed, and false otherwise
 */
template <AtomicFlag F, std::invocable A>
auto ExclusiveWithAtomicFlag(F& flag, A&& action)
    -> std::conditional_t<std::is_same_v<decltype(action()), void>, bool,
                          std::optional<decltype(action())>> {
  if constexpr (std::is_same_v<decltype(action()), void>) {
    if (!flag.test_and_set()) {
      ClearFlagAtExit<F> clear_flag{flag};
      action();
      return true;
    }
    return false;
  } else {
    if (!flag.test_and_set()) {
      ClearFlagAtExit clear_flag{flag};
      return action();
    }

    return {};
  }
}

/**
 * Initializes an atomic flag as an event. When the flag is set, the event is
 * handled, when it is cleared, the event is pending
 * @param flag Flag event to initialize
 */
void InitializeEvent(AtomicFlag auto& flag) noexcept {
  flag.test_and_set();
}

/**
 * Unconditionally pends an event
 * @param flag Flag to pend the event for
 * @return Whether the event pas pended
 */
void PendEvent(AtomicFlag auto& flag) noexcept {
  flag.clear();
}

/**
 * Tests the atomic event flag and marks it as handled, returns whether the
 * event was previously pending
 * @param flag Flag to test and handle
 * @return Whether there was an event that was marked as handled
 */
bool TestAndHandleEvent(AtomicFlag auto& flag) noexcept {
  return !flag.test_and_set();
}


}   // namespace halstd