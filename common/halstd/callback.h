#pragma once

#include <concepts>
#include <optional>
#include <tuple>
#include <utility>

#include "mp/types.h"

namespace halstd {

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

template <typename C, typename BA, typename CB>
class BoundDynamicMethodCallback;

template <typename T, typename... BoundArgs, typename... Args>
class BoundDynamicMethodCallback<T, Types<BoundArgs...>, Callback<Args...>>
    final : public Callback<Args...> {
  using MethodPtr = void (T::*)(BoundArgs..., Args...);

 public:
  ~BoundDynamicMethodCallback() noexcept final = default;

  explicit BoundDynamicMethodCallback(T* inst, MethodPtr ptr = nullptr)
      : inst{inst}
      , ptr{ptr} {}

  void operator()(Args... args) const noexcept final {
    if (inst != nullptr && bound_args.has_value()) {
      [this, &args...]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
        (inst->*ptr)(std::get<Idxs>(*bound_args)..., args...);
      }(std::make_index_sequence<sizeof...(BoundArgs)>());
    }
  }

  void RebindUnguarded(MethodPtr                new_method_ptr,
                       std::tuple<BoundArgs...> new_bound_args) noexcept {
    ptr        = new_method_ptr;
    bound_args = new_bound_args;
  }

 private:
  T*        inst;
  MethodPtr ptr;

  std::optional<std::tuple<BoundArgs...>> bound_args{};
};

template <typename... Args>
struct LambdaCallback {
  template <typename T>
  class Cb final
      : public T
      , public Callback<Args...> {
   public:
    explicit Cb(T&& t)
        : Callback<Args...>{}
        , T{t} {}

    ~Cb() final = default;

    void operator()(Args... args) const noexcept final {
      T::operator()(std::forward<Args>(args)...);
    }
  };

  template <typename T>
  Cb(T) -> Cb<T>;
};

}   // namespace halstd