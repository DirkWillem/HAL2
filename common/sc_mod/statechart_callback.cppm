module;

#include <utility>

export module statechart:callback;

import hstd;

import :core;

namespace sc {

template <typename SCR, typename E, typename... Args>
class ApplyEventCallback final : public hstd::Callback<Args...> {
  static_assert(IsStateChartRunner<SCR>);

public:
  explicit ApplyEventCallback(SCR& state_chart)
      : state_chart{state_chart}
  , event{} {}

  ~ApplyEventCallback() final = default;

  void operator()(Args...) const noexcept final {
    if (!state_chart.ApplyEvent(event)) {
      std::unreachable();
    }
  }

private:
  SCR& state_chart;
  E    event;
};

template <typename SCR, typename E, typename... Args>
class EnqueueEventCallback final : public hstd::Callback<Args...> {
  static_assert(IsStateChartRunner<SCR>);

public:
  explicit EnqueueEventCallback(SCR& state_chart)
      : state_chart{state_chart}
  , event{} {}

  ~EnqueueEventCallback() final = default;

  void operator()(Args...) const noexcept final {
    state_chart.EnqueueEvent(event);
  }

private:
  SCR& state_chart;
  E    event;
};

}   // namespace sc