module;

#include <chrono>
#include <utility>

export module hstd:chrono;

import :ratio;

export namespace hstd {
template <typename T>
struct is_duration : std::false_type {};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

template <typename T>
inline constexpr bool is_duration_v = is_duration<T>::value;

template <typename T>
/** Concept for std::chrono::duration */
concept Duration = is_duration_v<T>;

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

  using DefaultPeriodType = std::chrono::duration<Rep, ratio_reciprocal<Base>>;

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
  /**/
  [[nodiscard]] constexpr auto As() const noexcept {
    using mul = std::ratio_divide<B, typename F::Base>;

    return F{static_cast<typename F::Rep>((count * mul::num) / (mul::den))};
  }

  //  /** Three-way comparison operator */
  //  [[nodiscard]] constexpr std::strong_ordering operator<=>(Freq<R, B> rhs)
  //  const noexcept {
  //    return count <=> rhs.count;
  //  }

  [[nodiscard]] constexpr std::strong_ordering
  operator<=>(Frequency auto rhs) const noexcept {
    return count <=> rhs.template As<Freq<R, B>>().count;
  }

  [[nodiscard]] constexpr auto operator==(Freq<R, B> rhs) const noexcept {
    return (*this <=> rhs) == std::strong_ordering::equal;
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

/**
 * Helper class for passing duration literals as template arguments
 * @tparam Rep Duration representation
 * @tparam R Duration unit ratio
 */
template <std::unsigned_integral Rep, Ratio R>
struct DurationFactory {
  [[nodiscard]] constexpr Duration auto MakeDuration() const noexcept {
    return std::chrono::duration<Rep, R>{count};
  }

  constexpr auto operator+(const DurationFactory<Rep, R>& rhs) const noexcept {
    return DurationFactory<Rep, R>{count + rhs.count};
  }

  constexpr auto operator-(const DurationFactory<Rep, R>& rhs) const noexcept {
    return DurationFactory<Rep, R>{count - rhs.count};
  }

  constexpr auto operator*(Rep rhs) const noexcept {
    return DurationFactory<Rep, R>{count * rhs};
  }

  template <typename To>
  constexpr auto Cast() const noexcept {
    const auto result = std::chrono::duration_cast<To>(MakeDuration());
    return DurationFactory<typename To::rep, typename To::period>(
        result.count());
  }

  Rep count;
};
}   // namespace halstd

namespace hstd {

template <typename T>
inline constexpr bool IsDurationFactory = false;

template <typename Rep, typename R>
inline constexpr bool IsDurationFactory<DurationFactory<Rep, R>> = true;

}   // namespace halstd

export namespace hstd {
/**
 * Concept for values that either are a std::chrono::duration, or can create
 * one without additional input (i.e. DurationFactory)
 */
template <typename T>
concept ToDuration = Duration<T> || IsDurationFactory<T>;

/**
 * Creates a std::chrono::duration from a ToDuration value
 * @tparam T Value that can be converted into a duration
 * @param src Duration source
 * @return Instantiated duration
 */
template <ToDuration T>
[[nodiscard]] constexpr Duration auto MakeDuration(T src) noexcept {
  if constexpr (Duration<T>) {
    return src;
  } else {
    return src.MakeDuration();
  }
}

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

template <Ratio R>
constexpr auto DurationFactoryLit(unsigned long long int v) {
  if (v > std::numeric_limits<typename kHz::Rep>::max()) {
    std::unreachable();
  }

  return DurationFactory<uint32_t, R>{static_cast<uint32_t>(v)};
}

constexpr auto operator""_s(unsigned long long int v) {
  return DurationFactory<uint32_t, std::ratio<1, 1>>(v);
}

constexpr auto operator""_ms(unsigned long long int v) {
  return DurationFactory<uint32_t, std::milli>(v);
}

constexpr auto operator""_us(unsigned long long int v) {
  return DurationFactory<uint32_t, std::micro>(v);
}

}   // namespace literals

#if __cpp_lib_chrono >= 201907L
/**
 * Concept that wraps the std::chrono::is_clock condition
 */
template <typename C>
concept Clock = std::chrono::is_clock<C>;
#else
template<class>
struct is_clock : std::false_type {};

template<class T>
    requires
        requires
{
  typename T::rep;
  typename T::period;
  typename T::duration;
  typename T::time_point;
  T::is_steady; // type is not checked
  T::now();     // return type is not checked
}
struct is_clock<T> : std::true_type {};

/**
 * Concept that wraps the std::chrono::is_clock condition
 */
template<typename C>
concept Clock = is_clock<C>::value;
#endif

/**
 * Concept that describes a system clock that can block the current thread
 * of execution
 */
template <typename C>
concept SystemClock = Clock<C> && requires(C clk) {
  { clk.BlockFor(std::declval<typename C::duration>()) };
};

}   // namespace halstd