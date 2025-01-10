#pragma once

#include <hal/dma.h>
#include <hal/peripheral.h>

#include <stm32g0xx_hal.h>

#include <constexpr_tools/chrono_ex.h>
#include <constexpr_tools/values.h>

#include <stm32g0/clocks.h>
#include <stm32g0/dma.h>
#include <stm32g0/mappings/tim_pin_mapping.h>
#include <stm32g0/peripheral_ids.h>
#include <stm32g0/pin.h>

namespace stm32g0 {

using namespace ct::literals;

template <typename Impl>
concept TimChannel = requires(Impl impl) {
  { Impl::Channel } -> std::convertible_to<unsigned>;
  { Impl::IsPwm } -> std::convertible_to<bool>;
  { Impl::UsesDma } -> std::convertible_to<bool>;

  // { impl.template Configure<TimId{}>(1_kHz) };
};

template <typename Impl>
concept PeriodElapsedCallback = requires(Impl impl) {
  { impl.PeriodElapsedCallback() };
};

enum class PwmControlMode {
  Direct,
  Dma,
};

enum class PwmMode { Pwm1, Pwm2 };

namespace detail {

struct TimConfig {
  uint32_t arr;
  uint16_t prescaler;
};

consteval bool TimHasChannel(TimId tim, unsigned channel) {
  switch (tim) {
  // Advanced control timer with 6 channels
  case TimId::Tim1: return channel >= 1 && channel <= 6;
  // GP timers with 4 channels
  case TimId::Tim2: [[fallthrough]];
  case TimId::Tim3: [[fallthrough]];
  case TimId::Tim4: return channel >= 1 && channel <= 4;
  // GP timers with 1 channel
  case TimId::Tim14: return channel == 1;
  // GP timers with 2 channels
  case TimId::Tim15: [[fallthrough]];
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17: return channel >= 1 && channel <= 2;
  // Timers without channels or invalid TimId values
  default: return false;
  }
}

consteval bool TimChIsPwmWithOutputCapable(TimId tim, unsigned channel) {
  switch (tim) {
    // Advanced control timer with 6 channels
  case TimId::Tim1:
    return channel >= 1 && channel <= 4;
    // GP timers with 4 channels
  case TimId::Tim2: [[fallthrough]];
  case TimId::Tim3: [[fallthrough]];
  case TimId::Tim4:
    return channel >= 1 && channel <= 4;
    // GP timers with 1 channel
  case TimId::Tim14:
    return channel == 1;
    // GP timers with 2 channels
  case TimId::Tim15: [[fallthrough]];
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17:
    return channel >= 1 && channel <= 2;
    // Timers without channels or invalid TimId values
  default: return false;
  }
}

consteval bool TimChIsDmaCapable(TimId tim, unsigned channel) {
  switch (tim) {
    // Advanced control timer with 6 channels
  case TimId::Tim1:
    return channel >= 1 && channel <= 4;
    // GP timers with 4 channels
  case TimId::Tim2: [[fallthrough]];
  case TimId::Tim3: [[fallthrough]];
  case TimId::Tim4:
    return channel >= 1 && channel <= 4;
    // GP timers with 2 channels
  case TimId::Tim15: return channel >= 1 && channel <= 2;
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17:
    return channel == 1;
    // Timers without channels or invalid TimId values
  default: return false;
  }
}

void EnableTimClock(TimId id);
void EnableTimInterrupt(TimId id, uint32_t prio);

template <unsigned Ch, TimChannel... Chs>
struct GetByNumHelper;

template <unsigned Ch, TimChannel Cur, TimChannel... Rest>
struct GetByNumHelper<Ch, Cur, Rest...> {
  using Result =
      std::conditional_t<Cur::Channel == Ch, Cur,
                         typename GetByNumHelper<Ch, Rest...>::Result>;
};

template <unsigned Ch>
struct GetByNumHelper<Ch> {
  using Result = void;
};

template <TimChannel... Chs>
struct ChannelList {
  template <unsigned Ch>
  using GetByNum = typename GetByNumHelper<Ch, Chs...>::Result;
};

[[nodiscard]] consteval TimDmaRequest ChannelToDmaRequest(unsigned channel) {
  switch (channel) {
  case 1: return TimDmaRequest::Ch1;
  case 2: return TimDmaRequest::Ch2;
  case 3: return TimDmaRequest::Ch3;
  case 4: return TimDmaRequest::Ch4;
  default: std::unreachable();
  }
}

[[nodiscard]] constexpr uint32_t HalOcMode(PwmMode pm) noexcept {
  switch (pm) {
  case PwmMode::Pwm1: return TIM_OCMODE_PWM1;
  case PwmMode::Pwm2: return TIM_OCMODE_PWM2;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t HalChannel(unsigned ch) noexcept {
  switch (ch) {
  case 1: return TIM_CHANNEL_1;
  case 2: return TIM_CHANNEL_2;
  case 3: return TIM_CHANNEL_3;
  case 4: return TIM_CHANNEL_4;
  case 5: return TIM_CHANNEL_5;
  case 6: return TIM_CHANNEL_6;
  default: std::unreachable();
  }
}

}   // namespace detail

template <TimId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using TimPeriodElapsedDma = DmaChannel<Id, TimDmaRequest::PeriodElapsed, Prio>;

template <TimId Id, unsigned Ch, hal::DmaPriority Prio = hal::DmaPriority::Low>
using TimChDma = DmaChannel<Id, detail::ChannelToDmaRequest(Ch), Prio>;

template <unsigned Ch>
struct IsChannelX {
  template <TimChannel T>
  using Predicate = std::bool_constant<T::Ch == Ch>;
};

template <typename Impl>
concept TimConfigFn = requires(Impl f) {
  { f(1_kHz) } -> std::convertible_to<detail::TimConfig>;
};

template <unsigned Resolution, ct::Frequency auto FTimer>
/**
 * Timer configuration function that results in a fixed resolution and timer
 *   frequency
 */
inline constexpr auto FixedTimerResolutionAndFrequency =
    [](ct::Frequency auto f_src) {
      const auto f_ratio = f_src.template As<ct::Hz>().count
                           / FTimer.template As<ct::Hz>().count;
      const auto psc = f_ratio / Resolution - 1;

      return detail::TimConfig{.arr       = Resolution - 1,
                               .prescaler = static_cast<uint16_t>(psc)};
    };

static_assert(
    TimConfigFn<decltype(FixedTimerResolutionAndFrequency<100, 1_kHz>)>);

template <unsigned Ch, PinId P, PwmControlMode CM = PwmControlMode::Direct,
          PwmMode PM = PwmMode::Pwm1>
class TimPwmOutputChannel {
 public:
  static constexpr auto Channel = Ch;
  static constexpr auto IsPwm   = true;
  static constexpr auto UsesDma = CM == PwmControlMode::Dma;

  static constexpr auto ControlMode = CM;
  static constexpr auto PwmMode     = PM;

  template <TimId Tim>
  bool Configure([[maybe_unused]] ct::Frequency auto f_src) noexcept {
    static_assert(detail::TimHasChannel(Tim, Ch),
                  "Timer channel does not exist");
    static_assert(detail::TimChIsPwmWithOutputCapable(Tim, Ch),
                  "Timer channel is not capable of generating PWM with output");

    // Configure channel
    TIM_OC_InitTypeDef init{
        .OCMode     = detail::HalOcMode(PM),
        .Pulse      = 0,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };

    if (HAL_TIM_OC_ConfigChannel(&htim, &init, detail::HalChannel(Ch))
        != HAL_OK) {
      return false;
    }

    // Configure pin
    constexpr auto MappingOpt =
        hal::FindTimAFMapping(TimChPinMappings, Tim, Ch, P);
    static_assert(MappingOpt.has_value(),
                  "Pin is not valid for this timer channel");
    constexpr auto Mapping = *MappingOpt;

    Pin::InitializeAlternate(Mapping.pin, Mapping.af);

    return true;
  }

  template <TimId Tim>
  bool Configure(hal::Dma auto&                      dma,
                 [[maybe_unused]] ct::Frequency auto f_src) noexcept
    requires UsesDma
  {
    using DmaCh = TimPeriodElapsedDma<Tim>;

    // Call base Configure
    if (!Configure<Tim>(f_src)) {
      return false;
    }

    // Configure DMA
    auto& hdma = dma.template SetupChannel<DmaCh>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
        hal::DmaDataWidth::HalfWord, false, hal::DmaDataWidth::HalfWord, true);
    __HAL_LINKDMA(&htim, hdma[TIM_DMA_ID_UPDATE], hdma);

    // Enable timer interrupt
    detail::EnableTimInterrupt(Tim, 0);

    return true;
  }

  void Enable() {
    TIM_CCxChannelCmd(htim.Instance, detail::HalChannel(Ch), TIM_CCx_ENABLE);
  }

  void Disable() {
    TIM_CCxChannelCmd(htim.Instance, detail::HalChannel(Ch), TIM_CCx_DISABLE);
  }

  void compare(uint32_t new_cmp) noexcept {
    if constexpr (Ch == 1) {
      htim.Instance->CCR1 = new_cmp;
    } else if constexpr (Ch == 2) {
      htim.Instance->CCR2 = new_cmp;
    } else if constexpr (Ch == 3) {
      htim.Instance->CCR3 = new_cmp;
    } else if constexpr (Ch == 4) {
      htim.Instance->CCR4 = new_cmp;
    } else {
      std::unreachable();
    }
  }

  void SetDmaData(std::span<const uint16_t> cmps) {
    // HAL_TIM_PWM_Start_DMA(&htim, HalChannel(),
    //                       reinterpret_cast<const uint32_t*>(cmps.data()),
    //                       static_cast<uint16_t>(cmps.size()));

    const auto result = HAL_TIM_DMABurst_MultiWriteStart(
        &htim, TIM_DMABASE_CCR1 + (Ch - 1), TIM_DMA_UPDATE,
        reinterpret_cast<const uint32_t*>(cmps.data()),
        TIM_DMABURSTLENGTH_1TRANSFER, cmps.size());
    return;
    // HAL_TIM_PWM_Start(&htim, HalChannel());
  }

  void PeriodElapsedCallback() {
    HAL_TIM_DMABurst_WriteStop(&htim, TIM_DMA_UPDATE);
  }

 protected:
  explicit TimPwmOutputChannel(TIM_HandleTypeDef& htim)
      : htim{htim} {}

 private:
  TIM_HandleTypeDef& htim;
};

static_assert(TimChannel<TimPwmOutputChannel<2, PIN(A, 7)>>);
static_assert(PeriodElapsedCallback<TimPwmOutputChannel<2, PIN(A, 7)>>);

template <unsigned Ch0, ct::IsValues<PinId> Pins, ct::IsValues<PwmMode> PMs>
class TimMultiplePwmOutputChannel {
 public:
  static_assert(Pins::Count == PMs::Count);

  static constexpr auto NChs = Pins::Count;

  static constexpr auto Channel = Ch0;
  static constexpr auto IsPwm   = true;
  static constexpr auto UsesDma = true;

  template <TimId Tim>
  bool Configure(hal::Dma auto&                      dma,
                 [[maybe_unused]] ct::Frequency auto f_src) noexcept
    requires UsesDma
  {
    constexpr auto pins      = Pins::ToArray();
    constexpr auto pwm_modes = PMs::ToArray();

    // Configure channels
    if (!ConfigureChannels<Tim>(std::make_index_sequence<NChs>())) {
      return false;
    }

    // Configure DMA
    using DmaCh = TimPeriodElapsedDma<Tim>;

    auto& hdma = dma.template SetupChannel<DmaCh>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
        hal::DmaDataWidth::HalfWord, false, hal::DmaDataWidth::HalfWord, true);
    __HAL_LINKDMA(&htim, hdma[TIM_DMA_ID_UPDATE], hdma);

    // Enable timer interrupt
    detail::EnableTimInterrupt(Tim, 0);

    return true;
  }

  void Enable() {
    for (auto i = 0; i < NChs; i++) {
      TIM_CCxChannelCmd(htim.Instance, detail::HalChannel(Ch0 + i),
                        TIM_CCx_ENABLE);
    }
  }

  void Disable() {
    for (auto i = 0; i < NChs; i++) {
      TIM_CCxChannelCmd(htim.Instance, detail::HalChannel(Ch0 + i),
                        TIM_CCx_DISABLE);
    }
  }

  void SetCompares(std::array<uint16_t, NChs> cmps) {
    SetComparesImpl(cmps, std::make_index_sequence<NChs>());
  }

  bool SetDmaData(std::span<const uint16_t> cmps) {
    return HAL_TIM_DMABurst_MultiWriteStart(
               &htim, TIM_DMABASE_CCR1 + (Ch0 - 1), TIM_DMA_UPDATE,
               reinterpret_cast<const uint32_t*>(cmps.data()), (NChs - 1) << 8,
               cmps.size())
           == HAL_OK;
  }

  void PeriodElapsedCallback() {
    HAL_TIM_DMABurst_WriteStop(&htim, TIM_DMA_UPDATE);
  }

 protected:
  explicit TimMultiplePwmOutputChannel(TIM_HandleTypeDef& htim)
      : htim{htim} {}

 private:
  template <TimId Tim, std::size_t... Idxs>
  bool ConfigureChannels(std::index_sequence<Idxs...>) {
    return (... && ConfigureChannel<Tim, Idxs>());
  }

  template <TimId Tim, std::size_t I>
  bool ConfigureChannel() {
    constexpr auto Ch = Ch0 + I;

    static_assert(detail::TimHasChannel(Tim, Ch),
                  "Timer channel does not exist");
    static_assert(detail::TimChIsPwmWithOutputCapable(Tim, Ch),
                  "Timer channel is not capable of generating PWM with output");

    constexpr PinId   P  = Pins::GetByIndex(I);
    constexpr PwmMode PM = PMs::GetByIndex(I);

    // Configure channel
    TIM_OC_InitTypeDef init{
        .OCMode     = detail::HalOcMode(PM),
        .Pulse      = 0,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };

    if (HAL_TIM_OC_ConfigChannel(&htim, &init, detail::HalChannel(Ch))
        != HAL_OK) {
      return false;
    }

    // Configure pin
    constexpr auto MappingOpt =
        hal::FindTimAFMapping(TimChPinMappings, Tim, Ch, P);
    static_assert(MappingOpt.has_value(),
                  "Pin is not valid for this timer channel");
    constexpr auto Mapping = *MappingOpt;

    Pin::InitializeAlternate(Mapping.pin, Mapping.af);

    return true;
  }

  template <std::size_t... Idxs>
  void SetComparesImpl(std::array<uint16_t, NChs> new_cmps,
                       std::index_sequence<Idxs...>) {
    (..., SetCompare<Idxs>(new_cmps[Idxs]));
  }

  template <unsigned Idx>
  void SetCompare(uint32_t new_cmp) noexcept {
    constexpr auto Ch = Ch0 + Idx;
    if constexpr (Ch == 1) {
      htim.Instance->CCR1 = new_cmp;
    } else if constexpr (Ch == 2) {
      htim.Instance->CCR2 = new_cmp;
    } else if constexpr (Ch == 3) {
      htim.Instance->CCR3 = new_cmp;
    } else if constexpr (Ch == 4) {
      htim.Instance->CCR4 = new_cmp;
    } else {
      std::unreachable();
    }
  }

  TIM_HandleTypeDef& htim;
};

static_assert(TimChannel<TimMultiplePwmOutputChannel<
                  1, ct::Values<PinId, PIN(A, 6), PIN(A, 7)>,
                  ct::Values<PwmMode, PwmMode::Pwm2, PwmMode::Pwm1>>>);
static_assert(PeriodElapsedCallback<TimMultiplePwmOutputChannel<
                  1, ct::Values<PinId, PIN(A, 6), PIN(A, 7)>,
                  ct::Values<PwmMode, PwmMode::Pwm2, PwmMode::Pwm1>>>);

template <typename Impl, TimId Id, ClockSettings CS, TimChannel... Chs>
class TimImpl
    : public hal::UsedPeripheral
    , private Chs... {
  using ChannelNumbers = ct::Values<unsigned, Chs::Channel...>;
  using ChList         = detail::ChannelList<Chs...>;

  friend void ::HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);

 public:
  static constexpr auto UsesDma = (... || Chs::UsesDma);

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  void HandleInterrupt() { HAL_TIM_IRQHandler(&htim); }

  template <unsigned Ch>
  static consteval bool HasChannel() {
    return ChannelNumbers::Contains(Ch);
  }

  template <unsigned Ch>
  auto& Channel()
    requires(HasChannel<Ch>())
  {
    return *static_cast<typename ChList::template GetByNum<Ch>*>(this);
  }

  void Start() { HAL_TIM_Base_Start(&htim); }
  void Stop() { HAL_TIM_Base_Stop(&htim); }

 protected:
  explicit TimImpl(TimConfigFn auto config_fn)
    requires(!UsesDma)
      : Chs{htim}... {
    // Configure global timer
    constexpr ct::Frequency auto FSrc =
        CS.system_clock_settings.ApbTimersClockFrequency(
            CS.SysClkSourceClockFrequency());
    const auto settings = config_fn(FSrc);

    if (!ConfigureTimer(settings)) {
      return;
    }

    // Configure channels
    (..., Chs::template Configure<Id>(FSrc));
  }

  TimImpl(hal::Dma auto& dma, TimConfigFn auto config_fn)
    requires(UsesDma)
      : Chs{htim}... {
    // Configure global timer
    constexpr ct::Frequency auto FSrc =
        CS.system_clock_settings.ApbTimersClockFrequency(
            CS.SysClkSourceClockFrequency());
    const auto settings = config_fn(FSrc);

    if (!ConfigureTimer(settings)) {
      return;
    }

    // Configure channels
    (..., Configure<Chs>(dma, FSrc));
  }

  void PeriodElapsed() { (..., ChannelPeriodElapsedCallback<Chs>()); }

  TIM_HandleTypeDef htim{};

 private:
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
  void Configure(hal::Dma auto& dma, ct::Frequency auto f_src) {
    if constexpr (Ch::UsesDma) {
      Ch::template Configure<Id>(dma, f_src);
    } else {
      Ch::template Configure<Id>(f_src);
    }
  }

  template <TimChannel Ch>
    requires(... && !Chs::UsesDma)
  void Configure(ct::Frequency auto f_src) {
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