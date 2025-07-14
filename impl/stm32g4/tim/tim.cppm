module;

#include <cstdint>
#include <ratio>
#include <utility>

#include <stm32g4xx_hal.h>

export module hal.stm32g4:tim;

import hstd;

export import :tim.channel;
import :tim.common;
import :clocks;

extern "C" {

[[maybe_unused]] void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);
}

namespace stm32g4 {

using namespace hstd::literals;

export template <TimId Id, hal::DmaPriority Prio = hal::DmaPriority::Low,
                 typename Cb = hstd::Empty>
using TimUpdateDma = ::stm32g4::TimUpdateDma<Id, Prio, Cb>;

export struct TimConfig {
  uint32_t arr;
  uint16_t prescaler;
};

export template <typename Impl>
concept TimConfigFn = requires(Impl f) {
  { f(std::declval<TimId>(), 1_kHz) } -> std::convertible_to<TimConfig>;
};

/**
 * Timer configuration function that results in a fixed resolution and timer
 *   frequency
 */
export template <unsigned Resolution, hstd::Frequency auto FTimer>
inline constexpr auto FixedTimerResolutionAndFrequency =
    [](TimId, hstd::Frequency auto f_src) {
      const auto f_ratio = f_src.template As<hstd::Hz>().count
                           / FTimer.template As<hstd::Hz>().count;
      const auto psc = f_ratio / Resolution - 1;

      return TimConfig{.arr       = Resolution - 1,
                       .prescaler = static_cast<uint16_t>(psc)};
    };

export template <hstd::Frequency auto FTick>
inline constexpr auto FixedFrequencyMaxPeriod =
    [](TimId id, hstd::Frequency auto f_src) noexcept {
      const auto f_ratio = f_src.template As<hstd::Hz>().count
                           / FTick.template As<hstd::Hz>().count;

      return TimConfig{
          .arr       = MaxTimerPeriod(id),
          .prescaler = static_cast<uint16_t>(f_ratio - 1),
      };
    };

static_assert(
    TimConfigFn<decltype(FixedTimerResolutionAndFrequency<100, 1_kHz>)>);
static_assert(TimConfigFn<decltype(FixedFrequencyMaxPeriod<1_kHz>)>);

template <ClockSettings CS>
consteval auto TimerSourceClockFrequency(TimId id) {
  using enum TimId;

  switch (id) {
    // APB1 timers
  case Tim2: [[fallthrough]];
  case Tim3: [[fallthrough]];
  case Tim4: [[fallthrough]];
  case Tim5: [[fallthrough]];
  case Tim6: [[fallthrough]];
  case Tim7:
    return CS.system_clock_settings.Apb1TimersClockFrequency(
        CS.SysClkSourceClockFrequency());

    // APB2 timers
  case Tim1: [[fallthrough]];
  case Tim8: [[fallthrough]];
  case Tim15: [[fallthrough]];
  case Tim16: [[fallthrough]];
  case Tim17: [[fallthrough]];
  case Tim20:
    return CS.system_clock_settings.Apb2TimersClockFrequency(
        CS.SysClkSourceClockFrequency());
  }

  std::unreachable();
}

export template <typename Impl, TimId Id, ClockSettings CS,
                 TimConfigFn auto ConfigFn, TimChannel... Chs>
class TimImpl
    : public hal::UsedPeripheral
    , private Chs... {
  using ChannelNumbers = hstd::Values<unsigned, Chs::Channel...>;
  using ChList         = ChannelList<Chs...>;

  friend void ::HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);

 public:
  static constexpr auto UsesDma = (... || Chs::UsesDma);

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  void HandleInterrupt() noexcept { HAL_TIM_IRQHandler(&htim); }

  template <unsigned Ch>
  static consteval bool HasChannel() noexcept {
    return ChannelNumbers::Contains(Ch);
  }

  template <unsigned Ch>
  auto& Channel() noexcept
    requires(HasChannel<Ch>())
  {
    return *static_cast<typename ChList::template GetByNum<Ch>*>(this);
  }

  void Start() noexcept {
    if constexpr ((... || Chs::IsPwm)) {
      __HAL_TIM_MOE_ENABLE(&htim);
    }

    HAL_TIM_Base_Start(&htim);
  }

  void Stop() noexcept { HAL_TIM_Base_Stop(&htim); }

  bool StartWithInterrupt() noexcept {
    return HAL_TIM_Base_Start_IT(&htim) == HAL_OK;
  }

  void StopWithInterrupt() noexcept { HAL_TIM_Base_Stop_IT(&htim); }

  [[nodiscard]] uint32_t GetCounter() const noexcept {
    return htim.Instance->CNT;
  }

  void ResetCounter() noexcept { htim.Instance->CNT = 0; }

  void SetPeriod(uint32_t period) {
    htim.Instance->ARR = period;
    htim.Instance->EGR |= TIM_EGR_UG;
    htim.Instance->SR &= ~TIM_SR_UIF_Msk;
  }

  void SetRepetitions(uint8_t rep) {
    htim.Instance->RCR = static_cast<uint32_t>(rep);
  }

  uint32_t GetPeriod() const noexcept { return htim.Instance->ARR; }

  static constexpr hstd::Frequency auto Frequency() noexcept {
    constexpr auto OutFreqNumerator       = GetFrequencyRatioNumerator();
    constexpr hstd::Frequency auto FSrc   = TimerSourceClockFrequency<CS>(Id);
    constexpr auto                 Config = ConfigFn(Id, FSrc);
    const auto FHz = (FSrc / (Config.prescaler + 1)).template As<hstd::Hz>();

    return FHz.template As<
        hstd::Freq<hstd::Hz::Rep, std::ratio<OutFreqNumerator, 1>>>();
  }

  void EnableInterrupt() noexcept { EnableTimInterrupt(Id, 0); }
  void DisableInterrupt() noexcept { DisableTimInterrupt(Id); }

 protected:
  TimImpl()
    requires(!UsesDma)
      : Chs{htim}... {
    // Configure global timer
    const hstd::Frequency auto FSrc     = TimerSourceClockFrequency<CS>(Id);
    const auto                 settings = ConfigFn(Id, FSrc);

    if (!ConfigureTimer(settings)) {
      return;
    }

    // Configure channels
    (..., Chs::template Configure<Id>(FSrc));
  }

  explicit TimImpl(hal::Dma auto& dma)
    requires(UsesDma)
      : Chs{htim}... {
    // Configure global timer
    constexpr hstd::Frequency auto FSrc = TimerSourceClockFrequency<CS>(Id);

    const auto settings = ConfigFn(Id, FSrc);

    if (!ConfigureTimer(settings)) {
      return;
    }

    // Configure channels
    (..., Configure<Chs>(dma, FSrc));
  }

  void PeriodElapsed() {
    static_assert(hal::concepts::TimPeriodElapsedCallback<Impl>);
    if constexpr (hal::concepts::TimPeriodElapsedCallback<Impl>) {
      static_cast<Impl*>(this)->PeriodElapsedCallback();
    }
    (..., ChannelPeriodElapsedCallback<Chs>());
  }

  TIM_HandleTypeDef htim{};

 private:
  [[nodiscard]] static consteval std::intmax_t
  GetFrequencyRatioNumerator() noexcept {
    constexpr hstd::Frequency auto FSrc   = TimerSourceClockFrequency<CS>(Id);
    constexpr auto                 Config = ConfigFn(Id, FSrc);

    auto freq_in_current_unit =
        (FSrc / ((Config.prescaler + 1))).template As<hstd::Hz>().count;
    std::intmax_t numerator = 1;

    while (freq_in_current_unit >= 1'000) {
      numerator *= 1'000;
      freq_in_current_unit /= 1'000;
    }

    return numerator;
  }

  bool ConfigureTimer(const TimConfig& cfg) {
    // Enable clock
    EnableTimClock(Id);

    // Initialize timer
    htim.Instance = GetTimPointer(Id);
    htim.Init     = {
            .Prescaler         = cfg.prescaler,
            .CounterMode       = TIM_COUNTERMODE_UP,
            .Period            = cfg.arr,
            .ClockDivision     = TIM_CLOCKDIVISION_DIV1,
            .RepetitionCounter = 0,
            .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
    };
    if (HAL_TIM_Base_Init(&htim) != HAL_OK) {
      return false;
    }

    // Configure clock source
    TIM_ClockConfigTypeDef clk_config = {
        .ClockSource    = TIM_CLOCKSOURCE_INTERNAL,
        .ClockPolarity  = TIM_CLOCKPOLARITY_NONINVERTED,
        .ClockPrescaler = TIM_CLOCKPRESCALER_DIV1,
        .ClockFilter    = 0,
    };
    if (HAL_TIM_ConfigClockSource(&htim, &clk_config) != HAL_OK) {
      return false;
    }

    // Check if we have a PWM channel
    if ((false || ... || (Chs::IsPwm))) {
      if (HAL_TIM_PWM_Init(&htim) != HAL_OK) {
        return false;
      }
    }

    // Disable master/slave
    TIM_MasterConfigTypeDef master_config = {
        .MasterOutputTrigger  = TIM_TRGO_RESET,
        .MasterOutputTrigger2 = TIM_TRGO2_RESET,
        .MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE,
    };
    if (HAL_TIMEx_MasterConfigSynchronization(&htim, &master_config)
        != HAL_OK) {
      return false;
    }

    return true;
  }

  template <TimChannel Ch>
    requires(... || Chs::UsesDma)
  void Configure(hal::Dma auto& dma, hstd::Frequency auto f_src) {
    if constexpr (Ch::UsesDma) {
      Ch::template Configure<Id>(dma, f_src);
    } else {
      Ch::template Configure<Id>(f_src);
    }
  }

  template <TimChannel Ch>
    requires(... && !Chs::UsesDma)
  void Configure(hstd::Frequency auto f_src) {
    if constexpr (Ch::UsesDma) {
      std::unreachable();
    } else {
      Ch::template Configure<Id>(f_src);
    }
  }

  template <TimChannel Ch>
  void ChannelPeriodElapsedCallback() {
    if constexpr (concepts::ChannelPeriodElapsedCallback<Ch>) {
      Ch::ChannelPeriodElapsedCallback();
    }
  }
};

export template <TimId Id>
class Tim : public hal::UnusedPeripheral<Tim<Id>> {
  friend void ::HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);

 public:
  constexpr void PeriodElapsed() noexcept {}
  constexpr void HandleInterrupt() noexcept {}

 protected:
  TIM_HandleTypeDef htim{};
};

export using Tim1  = Tim<TimId::Tim1>;
export using Tim2  = Tim<TimId::Tim2>;
export using Tim3  = Tim<TimId::Tim3>;
export using Tim4  = Tim<TimId::Tim4>;
export using Tim5  = Tim<TimId::Tim5>;
export using Tim6  = Tim<TimId::Tim6>;
export using Tim7  = Tim<TimId::Tim7>;
export using Tim8  = Tim<TimId::Tim8>;
export using Tim15 = Tim<TimId::Tim15>;
export using Tim16 = Tim<TimId::Tim16>;
export using Tim17 = Tim<TimId::Tim17>;
export using Tim20 = Tim<TimId::Tim20>;

}   // namespace stm32g4