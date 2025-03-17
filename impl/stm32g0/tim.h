#pragma once

#include <hal/dma.h>
#include <hal/peripheral.h>
#include <hal/tim.h>

#include <stm32g0xx_hal.h>

#include <halstd/chrono_ex.h>
#include <halstd/mp/values.h>

#include <stm32g0/clocks.h>
#include <stm32g0/dma.h>
#include <stm32g0/peripheral_ids.h>

#include "tim/channel.h"
#include "tim/common.h"

namespace stm32g0 {

using namespace halstd::literals;

namespace detail {

struct TimConfig {
  uint32_t arr;
  uint16_t prescaler;
};

enum class TimerBitCount {
  Bits16,
  Bits32,
};

[[nodiscard]] constexpr TimerBitCount BitCount(TimId tim) noexcept {
  switch (tim) {
  case TimId::Tim1: return TimerBitCount::Bits16;
  case TimId::Tim2: return TimerBitCount::Bits32;
  case TimId::Tim3: [[fallthrough]];
  case TimId::Tim4: [[fallthrough]];
  case TimId::Tim6: [[fallthrough]];
  case TimId::Tim7: [[fallthrough]];
  case TimId::Tim14: [[fallthrough]];
  case TimId::Tim15: [[fallthrough]];
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17: return TimerBitCount::Bits16;
  default: std::unreachable();
  }
}

[[nodiscard]] constexpr uint32_t MaxTimerPeriod(TimId tim) noexcept {
  switch (BitCount(tim)) {
  case TimerBitCount::Bits16: return 0xFFFF;
  case TimerBitCount::Bits32: return 0xFFFFFFFF;
  default: std::unreachable();
  }
}

}   // namespace detail

template <TimId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using TimPeriodElapsedDma = DmaChannel<Id, TimDmaRequest::PeriodElapsed, Prio>;

template <typename Impl>
concept TimConfigFn = requires(Impl f) {
  { f(std::declval<TimId>(), 1_kHz) } -> std::convertible_to<detail::TimConfig>;
};

template <unsigned Resolution, halstd::Frequency auto FTimer>
/**
 * Timer configuration function that results in a fixed resolution and timer
 *   frequency
 */
inline constexpr auto FixedTimerResolutionAndFrequency =
    [](TimId, halstd::Frequency auto f_src) {
      const auto f_ratio = f_src.template As<halstd::Hz>().count
                           / FTimer.template As<halstd::Hz>().count;
      const auto psc = f_ratio / Resolution - 1;

      return detail::TimConfig{.arr       = Resolution - 1,
                               .prescaler = static_cast<uint16_t>(psc)};
    };

template <halstd::Frequency auto FTick>
inline constexpr auto FixedFrequencyMaxPeriod =
    [](TimId id, halstd::Frequency auto f_src) noexcept {
      const auto f_ratio = f_src.template As<halstd::Hz>().count
                           / FTick.template As<halstd::Hz>().count;

      return detail::TimConfig{
          .arr       = detail::MaxTimerPeriod(id),
          .prescaler = static_cast<uint16_t>(f_ratio - 1),
      };
    };

static_assert(
    TimConfigFn<decltype(FixedTimerResolutionAndFrequency<100, 1_kHz>)>);
static_assert(TimConfigFn<decltype(FixedFrequencyMaxPeriod<1_kHz>)>);

template <typename Impl, TimId Id, ClockSettings CS, TimConfigFn auto ConfigFn,
          TimChannel... Chs>
class TimImpl
    : public hal::UsedPeripheral
    , private Chs... {
  using ChannelNumbers = halstd::Values<unsigned, Chs::Channel...>;
  using ChList         = detail::ChannelList<Chs...>;

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

  void Start() noexcept { HAL_TIM_Base_Start(&htim); }
  void Stop() noexcept { HAL_TIM_Base_Stop(&htim); }

  void StartWithInterrupt() noexcept { HAL_TIM_Base_Start_IT(&htim); }
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

  static constexpr halstd::Frequency auto Frequency() noexcept {
    constexpr auto OutFreqNumerator = GetFrequencyRatioNumerator();
    constexpr halstd::Frequency auto FSrc =
        CS.system_clock_settings.ApbTimersClockFrequency(
            CS.SysClkSourceClockFrequency());
    constexpr auto Config = ConfigFn(Id, FSrc);
    const auto FHz = (FSrc / (Config.prescaler + 1)).template As<halstd::Hz>();

    return FHz.template As<
        halstd::Freq<halstd::Hz::Rep, std::ratio<OutFreqNumerator, 1>>>();
  }

  void EnableInterrupt() noexcept { detail::EnableTimInterrupt(Id, 0); }
  void DisableInterrupt() noexcept { detail::DisableTimInterrupt(Id); }

 protected:
  TimImpl()
    requires(!UsesDma)
      : Chs{htim}... {
    // Configure global timer
    constexpr halstd::Frequency auto FSrc =
        CS.system_clock_settings.ApbTimersClockFrequency(
            CS.SysClkSourceClockFrequency());
    const auto settings = ConfigFn(Id, FSrc);

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
    constexpr halstd::Frequency auto FSrc =
        CS.system_clock_settings.ApbTimersClockFrequency(
            CS.SysClkSourceClockFrequency());
    const auto settings = ConfigFn(Id, FSrc);

    if (!ConfigureTimer(settings)) {
      return;
    }

    // Configure channels
    (..., Configure<Chs>(dma, FSrc));
  }

  void PeriodElapsed() {
    if constexpr (hal::RegisterableTimPeriodElapsedCallback<Impl>) {
      static_cast<Impl*>(this)->InvokePeriodElapsedCallback();
    }
    (..., ChannelPeriodElapsedCallback<Chs>());
  }

  TIM_HandleTypeDef htim{};

 private:
  [[nodiscard]] static consteval std::intmax_t
  GetFrequencyRatioNumerator() noexcept {
    constexpr halstd::Frequency auto FSrc =
        CS.system_clock_settings.ApbTimersClockFrequency(
            CS.SysClkSourceClockFrequency());
    constexpr auto Config = ConfigFn(Id, FSrc);

    auto freq_in_current_unit =
        (FSrc / ((Config.prescaler + 1))).template As<halstd::Hz>().count;
    std::intmax_t numerator = 1;

    while (freq_in_current_unit >= 1'000) {
      numerator *= 1'000;
      freq_in_current_unit /= 1'000;
    }

    return numerator;
  }

  bool ConfigureTimer(const detail::TimConfig& cfg) {
    // Enable clock
    detail::EnableTimClock(Id);

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
        .ClockSource = TIM_CLOCKSOURCE_INTERNAL,
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
        .MasterOutputTrigger = TIM_TRGO_RESET,
        .MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE,
    };
    if (HAL_TIMEx_MasterConfigSynchronization(&htim, &master_config)
        != HAL_OK) {
      return false;
    }

    return true;
  }

  template <TimChannel Ch>
    requires(... || Chs::UsesDma)
  void Configure(hal::Dma auto& dma, halstd::Frequency auto f_src) {
    if constexpr (Ch::UsesDma) {
      Ch::template Configure<Id>(dma, f_src);
    } else {
      Ch::template Configure<Id>(f_src);
    }
  }

  template <TimChannel Ch>
    requires(... && !Chs::UsesDma)
  void Configure(halstd::Frequency auto f_src) {
    if constexpr (Ch::UsesDma) {
      std::unreachable();
    } else {
      Ch::template Configure<Id>(f_src);
    }
  }

  template <TimChannel Ch>
  void ChannelPeriodElapsedCallback() {
    if constexpr (PeriodElapsedCallback<Ch>) {
      Ch::PeriodElapsedCallback();
    }
  }
};

template <TimId Id>
class Tim : public hal::UnusedPeripheral<Tim<Id>> {
  friend void ::HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);

 public:
  constexpr void PeriodElapsed() noexcept {}
  constexpr void HandleInterrupt() noexcept {}

 protected:
  TIM_HandleTypeDef htim{};
};

using Tim1  = Tim<TimId::Tim1>;
using Tim2  = Tim<TimId::Tim2>;
using Tim3  = Tim<TimId::Tim3>;
using Tim4  = Tim<TimId::Tim4>;
using Tim6  = Tim<TimId::Tim6>;
using Tim7  = Tim<TimId::Tim7>;
using Tim14 = Tim<TimId::Tim14>;
using Tim15 = Tim<TimId::Tim15>;
using Tim16 = Tim<TimId::Tim16>;
using Tim17 = Tim<TimId::Tim17>;

}   // namespace stm32g0