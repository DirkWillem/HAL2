export module hal.abstract:clocks;

import hstd;

namespace hal {

export template <typename C>
concept ClockFrequencies = requires {
  { C::SysTickFrequency } -> hstd::Frequency;
};

}   // namespace hal
