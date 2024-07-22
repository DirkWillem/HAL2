#pragma once

#include <concepts>

namespace hal {

template<typename C>
concept ClockFrequencies = requires {
  { C::SysTickFrequency } -> std::convertible_to<uint32_t>;
};


}