module;

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>

export module hal.abstract:system;

import hstd;

namespace hal {

export template <typename Impl>
concept CriticalSectionInterface = requires {
  Impl::Enter();
  Impl::Exit();
};

export template <typename S>
concept System = requires {
  typename S::CriticalSectionInterface;
  requires CriticalSectionInterface<typename S::CriticalSectionInterface>;

  requires hstd::Atomic<typename S::template Atomic<int>>;
  typename S::AtomicFlag;

  typename S::Clock;
  requires hstd::SystemClock<typename S::Clock>;
};

export template <CriticalSectionInterface CSF>
class CriticalSection {
 public:
  CriticalSection() noexcept { CSF::Enter(); }
  ~CriticalSection() noexcept { CSF::Exit(); }

  CriticalSection(const CriticalSection&)            = delete;
  CriticalSection(CriticalSection&&)                 = delete;
  CriticalSection& operator=(const CriticalSection&) = delete;
  CriticalSection& operator=(CriticalSection&&)      = delete;
};

/**
 * @brief Performance statistics, keeping track of latest, min and max ticks
 * taken.
 */
export struct PerformanceStatistics {
  uint32_t ticks_latest   = 0;
  uint32_t ticks_min      = std::numeric_limits<uint32_t>::max();
  uint32_t ticks_max      = std::numeric_limits<uint32_t>::min();
  uint32_t n_measurements = 0;

  /**
   * @brief Updates the performance statistics.
   * @param ticks_new Latest ticks measurement.
   */
  void Update(uint32_t ticks_new) {
    ticks_latest = ticks_new;
    ticks_min    = std::min(ticks_min, ticks_new);
    ticks_max    = std::max(ticks_max, ticks_new);
    n_measurements++;
  }
};

export template <typename PT>
concept PerformanceTimer = requires {
  PT::Enable();
  { PT::Get() } -> std::convertible_to<uint32_t>;
};

}   // namespace hal
