#pragma once

#include <halstd_old/chrono_ex.h>

namespace hal {

template <typename C>
concept ClockFrequencies = requires {
  { C::SysTickFrequency } -> halstd::Frequency;
};


}   // namespace hal