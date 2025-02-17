#pragma once

#include <halstd/atomic.h>

#include "clocks.h"

namespace hal {

template <typename Impl>
concept CriticalSectionInterface = requires {
  Impl::Enter();
  Impl::Exit();
};

template <typename S>
concept System = requires {
  typename S::CriticalSectionInterface;
  requires CriticalSectionInterface<typename S::CriticalSectionInterface>;

  requires halstd::Atomic<typename S::template Atomic<int>>;
  typename S::AtomicFlag;

  typename S::Clock;
  requires Clock<typename S::Clock>;
};

template <CriticalSectionInterface CSF>
class CriticalSection {
 public:
  CriticalSection() noexcept { CSF::Enter(); }
  ~CriticalSection() noexcept { CSF::Exit(); }

  CriticalSection(const CriticalSection&)            = delete;
  CriticalSection(CriticalSection&&)                 = delete;
  CriticalSection& operator=(const CriticalSection&) = delete;
  CriticalSection& operator=(CriticalSection&&)      = delete;
};

}   // namespace hal