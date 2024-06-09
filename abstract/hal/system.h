#pragma once

namespace hal {

template <typename Impl>
concept CriticalSectionInterface = requires {
  Impl::Enter();
  Impl::Exit();
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