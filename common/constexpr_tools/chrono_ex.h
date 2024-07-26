#pragma once

#include <utility>

#include "std_ex.h"

namespace ct {

template <typename T>
/** Concept for std::ratio */
concept Ratio = stdex::is_ratio_v<T>;

template <typename T>
/** Concept for std::chrono::duration */
concept Duration = stdex::chrono::is_duration_v<T>;

template <typename T>
struct is_frequency : public std::false_type {};

template <typename T>
inline constexpr bool is_frequency_v = is_frequency<T>::value;

template <typename T>
/** Concept for frequencies */
concept Frequency = is_frequency_v<std::decay_t<T>>;

template <typename R, Ratio B = std::ratio<1, 1>>
/**
 * Represents a frequency
 * @tparam R Representation type
 * @tparam B Frequency base, as a multiplication of hertz
 */
class Freq {
 public:
  using Rep  = R;
  using Base = B;

  using DefaultPeriodType =
      std::chrono::duration<Rep, stdex::ratio_reciprocal<Base>>;

  constexpr explicit Freq(R count) noexcept
      : count{count} {}

  template <Duration D = DefaultPeriodType>
  /**
   * Returns the period of the frequency in the specified duration type
   * @tparam D Duration type
   * @return One period
   */
  [[nodiscard]] constexpr auto Period() const noexcept {
    if constexpr (std::ratio_equal_v<typename D::period,
                                     typename DefaultPeriodType::period>) {
      return D{static_cast<typename D::rep>(count)};
    } else {
      using mul = std::ratio_divide<typename DefaultPeriodType::period,
                                    typename D::period>;

      return D{static_cast<typename D::rep>((mul::num) / (count * mul::den))};
    }
  }

  template <Frequency F>
  /**
   * Casts the frequency to a different representation
   * @tparam F Frequency representation
   * @return Cast representation
   */
  [[nodiscard]] constexpr auto As() const noexcept {
    using mul = std::ratio_divide<B, typename F::Base>;

    return F{static_cast<typename F::Rep>((count * mul::num) / (mul::den))};
  }

  /** Three-way comparison operator */
  [[nodiscard]] constexpr auto operator<=>(Freq<R, B> rhs) const noexcept {
    return count <=> rhs.count;
  }

  /** Compound addition operator */
  constexpr Freq& operator+=(Freq<R, B> rhs) noexcept {
    count += rhs.count;
    return *this;
  }

  /** Addition operator */
  [[nodiscard]] constexpr auto operator+(Freq<R, B> rhs) const noexcept {
    auto result = *this;
    result += rhs;
    return result;
  }

  /** Compound subtraction operator */
  constexpr Freq& operator-=(Freq<R, B> rhs) noexcept {
    count -= rhs.count;
    return *this;
  }

  /** Subtraction operator */
  [[nodiscard]] constexpr auto operator-(Freq<R, B> rhs) const noexcept {
    auto result = *this;
    result -= rhs;
    return result;
  }

  /** Compound scalar multiplication operator */
  constexpr Freq& operator*=(R rhs) noexcept {
    count *= rhs;
    return *this;
  }

  /** Scalar multiplication operator */
  [[nodiscard]] constexpr auto operator*(R rhs) const noexcept {
    auto result = *this;
    result *= rhs;
    return result;
  }

  /** Scalar division operator */
  [[nodiscard]] constexpr auto operator/(R rhs) const noexcept {
    auto result = *this;
    result /= rhs;
    return result;
  }
  /** Compound scalar division operator */
  constexpr Freq& operator/=(R rhs) noexcept {
    count /= rhs;
    return *this;
  }

  R count{};
};

template <typename R, typename B>
struct is_frequency<Freq<R, B>> : std::true_type {};

using hertz     = Freq<uint32_t, std::ratio<1, 1>>;
using kilohertz = Freq<uint32_t, std::kilo>;
using megahertz = Freq<uint32_t, std::mega>;
using gigahertz = Freq<uint32_t, std::giga>;

using Hz  = hertz;
using kHz = kilohertz;
using MHz = megahertz;
using GHz = gigahertz;

namespace literals {

template <Frequency F>
constexpr auto FreqLit(unsigned long long int v) {
  if (v > std::numeric_limits<typename kHz::Rep>::max()) {
    std::unreachable();
  }

  return F{static_cast<uint32_t>(v)};
}

constexpr auto operator""_Hz(unsigned long long int v) {
  return FreqLit<hertz>(v);
}

constexpr auto operator""_kHz(unsigned long long int v) {
  return FreqLit<kilohertz>(v);
}

constexpr auto operator""_MHz(unsigned long long int v) {
  return FreqLit<megahertz>(v);
}

}   // namespace literals

}   // namespace ct
