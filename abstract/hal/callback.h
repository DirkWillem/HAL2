#pragma once

#include <utility>

namespace hal {

template <typename... Args>
class Callback {
 public:
  virtual ~Callback() = default;

  virtual void Invoke(Args...) const noexcept = 0;
};

template <typename T, typename... Args>
class MethodCallback final : public Callback<Args...> {
  using MethodPtr = void (T::*)(Args...);

 public:
  ~MethodCallback() final = default;

  explicit MethodCallback(T* inst, MethodPtr ptr)
      : inst{inst}, ptr{ptr} {}

  void Invoke(Args... args) const noexcept final { (inst->*ptr)(args...); }

 private:
  T*        inst;
  MethodPtr ptr;
};

}   // namespace hal