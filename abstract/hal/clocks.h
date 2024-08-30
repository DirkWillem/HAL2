#pragma once

#include <constexpr_tools/chrono_ex.h>

namespace hal {

template <typename C>
concept ClockFrequencies = requires {
  { C::SysTickFrequency } -> ct::Frequency;
};

template <typename C>
concept Clock = requires {
  { C::TimeSinceBoot() } -> ct::Duration;
};

}   // namespace hal