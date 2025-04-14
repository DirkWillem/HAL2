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

}   // namespace hal
