module;

#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <span>

export module hal.abstract:tim;

import hstd;

namespace hal {

/**
 * Concept for timer
 * @tparam Impl Implementation type
 */
export template <typename Impl>
concept Tim = requires(Impl& impl, const Impl& cimpl) {
  impl.Start();
  impl.StartWithInterrupt();
  impl.Stop();
  impl.StopWithInterrupt();

  { Impl::Frequency() } -> hstd::Frequency;
  requires hstd::IsConstantExpression<Impl::Frequency()>();

  impl.SetPeriod(std::declval<uint32_t>());

  { cimpl.GetCounter() } -> std::convertible_to<uint32_t>;
  impl.ResetCounter();

  impl.EnableInterrupt();
  impl.DisableInterrupt();
};

/**
 * Concept for a registerable timer period elapsed callback
 */
export template <typename Impl>
concept RegisterableTimPeriodElapsedCallback = requires(Impl& impl) {
  impl.RegisterPeriodElapsedCallback(std::declval<hstd::Callback<>&>());
  impl.ClearPeriodElapsedCallback();
  impl.InvokePeriodElapsedCallback();
};

/**
 * Concept Burst DMA PWM channel
 * @tparam Impl Implementation type
 * @tparam N Number of channel compares to write over burst DMA
 */
template <typename Impl, std::size_t N>
concept BurstDmaPwmChannel = requires(Impl& impl) {
  impl.SetCompares(std::declval<std::array<uint16_t, N>>());
  {
    impl.SetDmaData(std::declval<const std::span<uint16_t>>())
  } -> std::convertible_to<bool>;

  impl.Enable();
  impl.Disable();
};

/**
 * Implementation of std::chrono clock using a hardware timer
 * @tparam T Timer typer
 * @tparam FClock Clock Frequency
 */
export template <Tim T, hstd::Frequency auto FClock>
struct TimClock {
  using duration   = typename std::decay_t<decltype(FClock)>::DefaultPeriodType;
  using rep        = typename duration::rep;
  using period     = typename duration::period;
  using time_point = std::chrono::time_point<TimClock<T, FClock>, duration>;

  static constexpr auto is_steady = false;

  static time_point now() noexcept {
    constexpr auto FTim = T::Frequency().template As<hstd::Hz>();
    constexpr auto FClk = FClock.template As<hstd::Hz>();

    static_assert(FTim <= FClk);

    constexpr auto Ratio = FClk.count / FTim.count;
    return time_point{duration{T::instance().GetCounter() * Ratio}};
  }
};

export template <typename Impl>
concept AlarmTim = Tim<Impl> && RegisterableTimPeriodElapsedCallback<Impl>;

template <AlarmTim T>
class AlarmImpl {
 protected:
  explicit AlarmImpl(const hstd::Callback<>& alarm_cb) noexcept
      : callback{this, &AlarmImpl::PeriodElapsedCallback}
      , inner_callback{&alarm_cb} {
    T::instance().RegisterPeriodElapsedCallback(callback);
  }

  ~AlarmImpl() noexcept {
    if (inner_callback != nullptr) {
      AlarmTim auto& tim = T::instance();
      tim.DisableInterrupt();
      tim.ClearPeriodElapsedCallback();
    }
  }

  /**
   * Move contructor
   * @param rhs Moved value
   */
  AlarmImpl(AlarmImpl&& rhs) noexcept
      : callback{this, &AlarmImpl::PeriodElapsedCallback}
      , inner_callback{rhs.inner_callback} {
    rhs.inner_callback = nullptr;
    T::instance().RegisterPeriodElapsedCallback(callback);
  }

  /**
   * Move assignemnt operator
   * @param rhs Assigned instance
   * @return Current instance
   */
  AlarmImpl& operator=(AlarmImpl&& rhs) noexcept {
    callback           = rhs.callback;
    rhs.inner_callback = nullptr;
    T::instance().RegisterPeriodElapsedCallback(callback);
    return *this;
  }

  AlarmImpl(const AlarmImpl&)            = delete;
  AlarmImpl& operator=(const AlarmImpl&) = delete;

  void StartImpl(uint32_t period) noexcept {
    AlarmTim auto& tim = T::instance();

    tim.StopWithInterrupt();
    tim.SetPeriod(period);
    tim.ResetCounter();
    tim.EnableInterrupt();

    tim.StartWithInterrupt();
  }

  /**
   * Cancels the alarm
   */
  void Cancel() noexcept {
    AlarmTim auto& tim = T::instance();
    tim.StopWithInterrupt();
  }

 private:
  void PeriodElapsedCallback() {
    if (inner_callback != nullptr) {
      AlarmTim auto& tim = T::instance();
      tim.Stop();

      (*inner_callback)();
    }
  }

  hstd::MethodCallback<AlarmImpl<T>> callback;
  const hstd ::Callback<>*           inner_callback;
};

/**
 * Class that uses a timer to implemented an alarm, i.e. a callback that is
 * invoked from an interrupt context after a fixed amount of time from starting
 * the alarm
 * @tparam T Timer instance
 * @tparam TO Timeout value, see halstd::DurationFactory
 */
export template <AlarmTim T, hstd::ToDuration auto TO>
class Alarm : private AlarmImpl<T> {
 public:
  /**
   * Constructor
   * @param alarm_cb Alarm callback
   * @param autostart Whether to immediately start the alarm
   */
  explicit Alarm(const hstd::Callback<>& alarm_cb,
                 bool                    autostart = false) noexcept
      : AlarmImpl<T>{alarm_cb} {
    if (autostart) {
      Start();
    }
  }

  /**
   * Starts the alarm
   */
  void Start() noexcept {
    constexpr auto TimFreq   = T::Frequency();
    constexpr auto TimPeriod = TimFreq.Period();
    constexpr auto Timeout =
        std::chrono::duration_cast<std::decay_t<decltype(TimPeriod)>>(
            hstd::MakeDuration(TO));
    constexpr auto Period = Timeout.count() / TimPeriod.count() - 1;

    AlarmImpl<T>::StartImpl(Period);
  }

  using AlarmImpl<T>::Cancel;
};

/**
 * Base class that can be inherited from to automatically implement
 * RegisterableTimPeriodElapsedCallback
 */
export class TimPeriodElapsedCallback {
 public:
  constexpr void InvokePeriodElapsedCallback() noexcept {
    if (callback != nullptr) {
      (*callback)();
    }
  }

  constexpr void
  RegisterPeriodElapsedCallback(hstd::Callback<>& new_callback) noexcept {
    callback = &new_callback;
  }

  constexpr void ClearPeriodElapsedCallback() noexcept { callback = nullptr; }

 private:
  hstd::Callback<>* callback{nullptr};
};

}   // namespace hal
