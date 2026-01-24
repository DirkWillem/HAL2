module;

#include <chrono>
#include <cstdint>
#include <utility>

#include <stm32h5xx_hal.h>

export module hal.stm32h5:tim;

import hstd;
import hal.abstract;

import :peripherals;
import :clocks;

namespace stm32h5 {

using namespace hstd::literals;

export enum class TimType {
  Advanced,
  GeneralPurpose32Bit,
  GeneralPurpose16Bit,
  Basic,
  SingleChannel,
  DualChannel,
  SingleChannelWithComplementary,
  DualChannelWithComplementary,
};

export constexpr TimType GetTimerType(TimId tim) noexcept {
  using enum TimId;
  using enum TimType;

  switch (tim) {
  case Tim1: return Advanced;
  case Tim2: return GeneralPurpose32Bit;
  case Tim3: return GeneralPurpose16Bit;
  case Tim4: return GeneralPurpose16Bit;
  case Tim5: return GeneralPurpose32Bit;
  case Tim6: return Basic;
  case Tim7: return Basic;
  case Tim8: return Advanced;
  case Tim12: return DualChannel;
  case Tim15: return DualChannelWithComplementary;
  default: std::unreachable();
  }
}

export enum class TimerBitCount {
  Bits16,
  Bits32,
};

export [[nodiscard]] constexpr TimerBitCount BitCount(TimId tim) noexcept {
  using enum TimType;

  switch (GetTimerType(tim)) {
  case GeneralPurpose32Bit: return TimerBitCount::Bits32;
  default: return TimerBitCount::Bits16;
  }
}

[[nodiscard]] constexpr uint32_t MaxTimerPeriod(TimId tim) noexcept {
  switch (BitCount(tim)) {
  case TimerBitCount::Bits16: return 0xFFFFU;
  case TimerBitCount::Bits32: return 0xFFFF'FFFF;
  default: std::unreachable();
  }
}

export struct TimConfig {
  uint32_t arr;
  uint16_t prescaler;
};

export template <typename Impl>
concept TimConfigFn = requires(Impl f) {
  { f(std::declval<TimId>(), 1_kHz) } -> std::convertible_to<TimConfig>;
};

/**
 * @brief Timer configuration function that results in a fixed resolution and
 * timer frequency
 *
 * @tparam Resolution Desired resolution.
 * @tparam FTimer Desired timer frequency.
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

/**
 * @brief Timer configuration function that results in a fixed timer frequency
 * with the maximum possible resolution.
 *
 * @tparam FTick Desired timer frequency.
 */
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
  case Tim7: [[fallthrough]];
  case Tim12:
    return CS.system_clock_settings.Apb1TimersClockFrequency(
        CS.SysClkSourceClockFrequency());

    // APB2 timers
  case Tim1: [[fallthrough]];
  case Tim8: [[fallthrough]];
  case Tim15:
    [[fallthrough]];
    return CS.system_clock_settings.Apb2TimersClockFrequency(
        CS.SysClkSourceClockFrequency());
  }

  std::unreachable();
}

/**
 * @brief Base class for timer implementations.
 * @tparam Impl Implementing type.
 * @tparam Id Timer ID.
 * @tparam CS Clock settings.
 * @tparam ConfigFn Timer configuration function.
 */
export template <typename Impl, TimId Id, ClockSettings CS,
                 TimConfigFn auto ConfigFn>
class TimImpl : public hal::UsedPeripheral {
 public:
  /**
   * @brief Returns the singleton instance of the timer.
   * @return Singleton instance.
   */
  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  /**
   * @brief Starts the timer.
   */
  void Start() { HAL_TIM_Base_Start(&htim); }

  /**
   * @brief Resets the timer counter.
   */
  void ResetCounter() noexcept { htim.Instance->CNT = 0; }

  /**
   * @brief Gets the current counter value.
   * @return Current counter value.
   */
  uint32_t GetCounter() const noexcept { return htim.Instance->CNT; }

  /**
   * @brief Returns the counter period in ticks.
   * @return Counter period, in ticks.
   */
  static uint32_t GetCounterPeriod() noexcept { return GetConfig().arr; }

  /**
   * @brief Returns the tick period (i.e. duration of a single tick) in the
   * requested duration representation.
   * @tparam D Duration type to represent the tick period in.
   * @return Duration of a single timer tick.
   */
  template <hstd::Duration D = std::chrono::duration<uint32_t, std::milli>>
  static constexpr hstd::Duration auto TickPeriod() noexcept {
    return TickFrequency().template Period<D>();
  }

  /**
   * @brief Returns the timer tick frequency.
   * @return Tick frequency.
   */
  static constexpr hstd::Frequency auto TickFrequency() noexcept {
    constexpr auto OutFreqNumerator       = GetFrequencyRatioNumerator();
    constexpr hstd::Frequency auto FSrc   = TimerSourceClockFrequency<CS>(Id);
    constexpr auto                 Config = ConfigFn(Id, FSrc);
    const auto FHz = (FSrc / (Config.prescaler + 1)).template As<hstd::Hz>();

    return FHz.template As<
        hstd::Freq<hstd::Hz::Rep, std::ratio<OutFreqNumerator, 1>>>();
  }

  /**
   * @brief Returns the maximum duration representable by the timer, in the
   * requested duration representation.
   * @tparam D Duration type to represent the tick period in.
   * @return Duration of a single overflow of the counter.
   */
  template <hstd::Duration D = std::chrono::duration<uint32_t, std::milli>>
  static constexpr hstd::Duration auto Period() noexcept {
    return TickPeriod<D>() * GetCounterPeriod();
  }

 protected:
  TimImpl() {
    // Configure global timer
    const auto settings = GetConfig();

    if (!ConfigureTimer(settings)) {
      return;
    }
  }

  TIM_HandleTypeDef htim{};

 private:
  static constexpr TimConfig GetConfig() noexcept {
    const hstd::Frequency auto FSrc = TimerSourceClockFrequency<CS>(Id);
    return ConfigFn(Id, FSrc);
  }

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

  static void EnableTimClock(TimId tim) {
    using enum TimId;
    switch (tim) {
    case Tim1: __HAL_RCC_TIM1_CLK_ENABLE(); break;
    case Tim2: __HAL_RCC_TIM2_CLK_ENABLE(); break;
    case Tim3: __HAL_RCC_TIM3_CLK_ENABLE(); break;
    case Tim4: __HAL_RCC_TIM4_CLK_ENABLE(); break;
    case Tim5: __HAL_RCC_TIM5_CLK_ENABLE(); break;
    case Tim6: __HAL_RCC_TIM6_CLK_ENABLE(); break;
    case Tim7: __HAL_RCC_TIM7_CLK_ENABLE(); break;
    case Tim8: __HAL_RCC_TIM8_CLK_ENABLE(); break;
    case Tim12: __HAL_RCC_TIM12_CLK_ENABLE(); break;
    case Tim15: __HAL_RCC_TIM15_CLK_ENABLE(); break;
    }
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
    const TIM_ClockConfigTypeDef clk_config = {
        .ClockSource    = TIM_CLOCKSOURCE_INTERNAL,
        .ClockPolarity  = TIM_CLOCKPOLARITY_NONINVERTED,
        .ClockPrescaler = TIM_CLOCKPRESCALER_DIV1,
        .ClockFilter    = 0,
    };
    if (HAL_TIM_ConfigClockSource(&htim, &clk_config) != HAL_OK) {
      return false;
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
};

export template <TimId Id>
class Tim : public hal::UnusedPeripheral<Tim<Id>> {
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
export using Tim12 = Tim<TimId::Tim12>;
export using Tim15 = Tim<TimId::Tim15>;

}   // namespace stm32h5