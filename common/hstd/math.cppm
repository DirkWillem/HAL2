module;

#include <concepts>

export module hstd:math;


namespace hstd {

export [[nodiscard]] constexpr bool
IsPowerOf2(std::unsigned_integral auto v) noexcept {
  return v && ((v & (v - 1)) == 0);
}

}