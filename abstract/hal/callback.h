#pragma once

#include <concepts>
#include <utility>

namespace hal {

template <typename... Args>
class Callback {
 public:
  virtual ~Callback() noexcept = default;

  virtual void operator()(Args...) const noexcept = 0;
};

template <typename T, typename... Args>
class MethodCallback final : public Callback<Args...> {
  using MethodPtr = void (T::*)(Args...);

 public:
  ~MethodCallback() noexcept final = default;

  MethodCallback(T* inst, MethodPtr ptr)
      : inst{inst}
      , ptr{ptr} {}

  void operator()(Args... args) const noexcept final { (inst->*ptr)(args...); }

 private:
  T*        inst;
  MethodPtr ptr;
};

}   // namespace hal