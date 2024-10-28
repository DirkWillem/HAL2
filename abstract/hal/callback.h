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

  /**
   * Re-binds the method callback to a different method pointer.
   *
   * @note This is not guarded against race conditions, such as the method being
   * invoked during the rebinding process. The user should ensure these kinds of
   * edge cases cannot occur
   */
  void RebindUnguarded(MethodPtr new_method_ptr) noexcept {
    ptr = new_method_ptr;
  }

 private:
  T*        inst;
  MethodPtr ptr;
};

template <typename T, typename... Args>
class DynamicMethodCallback final : public Callback<Args...> {
  using MethodPtr = void (T::*)(Args...);

 public:
  ~DynamicMethodCallback() noexcept final = default;

  explicit DynamicMethodCallback(T* inst, MethodPtr ptr = nullptr)
      : inst{inst}
      , ptr{ptr} {}

  void operator()(Args... args) const noexcept final {
    if (ptr != nullptr) {
      (inst->*ptr)(args...);
    }
  }

  /**
   * Re-binds the method callback to a different method pointer.
   *
   * @note This is not guarded against race conditions, such as the method being
   * invoked during the rebinding process. The user should ensure these kinds of
   * edge cases cannot occur
   */
  void RebindUnguarded(MethodPtr new_method_ptr) noexcept {
    ptr = new_method_ptr;
  }

 private:
  T*        inst;
  MethodPtr ptr;
};

}   // namespace hal