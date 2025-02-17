#pragma once

#include <halstd/callback.h>

#include "statechart.h"

namespace sc {

template <typename SCR, typename E, typename... Args>
class ApplyEventCallback final : halstd::Callback<Args...> {
  static_assert(IsStateChartRunner<SCR>);

 public:
  explicit ApplyEventCallback(SCR& state_chart)
      : state_chart{state_chart}
      , event{} {}

  void operator()(Args...) const noexcept final {
    if (!state_chart.ApplyEvent(event)) {
      std::unreachable();
    }
  }

 private:
  SCR& state_chart;
  E    event;
};

}   // namespace sc