#pragma once

#include <constexpr_tools/chrono_ex.h>

namespace hal {

template <typename C>
concept ClockFrequencies = requires {
  { C::SysTickFrequency } -> ct::Frequency;
};

template <typename C>
concept Clock = requires {
  requires ct::Duration<typename C::DurationType>;

  { C::TimeSinceBoot() } -> std::convertible_to<typename C::DurationType>;
  { C::BlockFor(std::declval<typename C::DurationType>()) };
};

}   // namespace hal