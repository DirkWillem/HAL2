module;

#include <span>
#include <type_traits>
#include <utility>

#include <stm32g0xx_hal.h>

#include <pin_macros.h>

export module hal.stm32g0:tim.channel;

import hstd;

import :pin;
import :tim.common;
import :pin_mapping.tim;

namespace stm32g0 {

/**
 * Concept describing a timer channel
 */
template <typename Impl>
concept TimChannel = requires(Impl impl) {
  { Impl::Channel } -> std::convertible_to<unsigned>;
  { Impl::IsPwm } -> std::convertible_to<bool>;
  { Impl::UsesDma } -> std::convertible_to<bool>;
};

/**
 * Modes for controlling PWM on a timer channel
 */
enum class PwmControlMode {
  Direct,
  Dma,
};

/**
 * Possible PWM modes, for details refer to STM32 datasheet
 */
enum class PwmMode { Pwm1, Pwm2 };

namespace detail {

/**
 * Returns whether a timer has a given channel
 * @param tim Timer to check for
 * @param channel Timer channel to check
 * @return Whether the given timer has the given channel
 */
consteval bool TimHasChannel(TimId tim, unsigned channel) noexcept {
  switch (tim) {
  // Advanced control timer with 6 channels
  case TimId::Tim1:
    return channel >= 1 && channel <= 6;
    // GP timers with 4 channels
#ifdef HAS_TIM2
  case TimId::Tim2: [[fallthrough]];
#endif
#ifdef HAS_TIM4
  case TimId::Tim4: [[fallthrough]];
#endif
  case TimId::Tim3: return channel >= 1 && channel <= 4;
  // GP timers with 1 channel
  case TimId::Tim14:
    return channel == 1;
    // GP timers with 2 channels
#ifdef HAS_TIM15
  case TimId::Tim15: [[fallthrough]];
#endif
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17: return channel >= 1 && channel <= 2;
  // Timers without channels or invalid TimId values
  default: return false;
  }
}

/**
 * Returns whether a timer channel is capable of PWM output
 * @param tim Timer to check
 * @param channel Channel to check
 * @return Whether the channel can output PWM
 */
consteval bool TimChIsPwmWithOutputCapable(TimId    tim,
                                           unsigned channel) noexcept {
  switch (tim) {
    // Advanced control timer with 6 channels
  case TimId::Tim1:
    return channel >= 1 && channel <= 4;
    // GP timers with 4 channels
#ifdef HAS_TIM2
  case TimId::Tim2: [[fallthrough]];
#endif
#ifdef HAS_TIM4
  case TimId::Tim4: [[fallthrough]];
#endif
  case TimId::Tim3:
    return channel >= 1 && channel <= 4;
    // GP timers with 1 channel
  case TimId::Tim14:
    return channel == 1;
    // GP timers with 2 channels
#ifdef HAS_TIM15
  case TimId::Tim15: [[fallthrough]];
#endif
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17:
    return channel >= 1 && channel <= 2;
    // Timers without channels or invalid TimId values
  default: return false;
  }
}

/**
 * Returns whether a given timer channel is DMA capable
 * @param tim Timer to check
 * @param channel Channel to check
 * @return Whether the given channel is DMA capable
 */
consteval bool TimChIsDmaCapable(TimId tim, unsigned channel) noexcept {
  switch (tim) {
    // Advanced control timer with 6 channels
  case TimId::Tim1:
    return channel >= 1 && channel <= 4;
    // GP timers with 4 channels
#ifdef HAS_TIM2
  case TimId::Tim2: [[fallthrough]];
#endif
#ifdef HAS_TIM4
  case TimId::Tim4: [[fallthrough]];
#endif
  case TimId::Tim3:
    return channel >= 1 && channel <= 4;
    // GP timers with 2 channels
#ifdef HAS_TIM15
  case TimId::Tim15: return channel >= 1 && channel <= 2;
#endif
  case TimId::Tim16: [[fallthrough]];
  case TimId::Tim17:
    return channel == 1;
    // Timers without channels or invalid TimId values
  default: return false;
  }
}

/**
 * Predicate for checking whether a timer channel is the channel of the given
 * number
 * @tparam Ch Channel to check\
 */
template <unsigned Ch>
struct IsChannelX {
  template <TimChannel T>
  using Predicate = std::bool_constant<T::Ch == Ch>;
};

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

template <TimId Id, unsigned Ch, hal::DmaPriority Prio = hal::DmaPriority::Low>
using TimChDma = DmaChannel<Id, detail::ChannelToDmaRequest(Ch), Prio>;

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
  bool Configure([[maybe_unused]] hstd::Frequency auto f_src) noexcept {
    static_assert(detail::TimHasChannel(Tim, Ch),
                  "Timer channel does not exist");
    static_assert(detail::TimChIsPwmWithOutputCapable(Tim, Ch),
                  "Timer channel is not capable of generating PWM with output");

    // Configure channel
    TIM_OC_InitTypeDef init{
        .OCMode       = detail::HalOcMode(PM),
        .Pulse        = 0,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCFastMode   = TIM_OCFAST_DISABLE,
        .OCIdleState  = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
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
  bool Configure(hal::Dma auto&                        dma,
                 [[maybe_unused]] hstd::Frequency auto f_src) noexcept
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
    EnableTimInterrupt(Tim, 0);

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
    (void)result;
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

template <unsigned Ch0, hstd::IsValues<PinId> Pins, hstd::IsValues<PwmMode> PMs>
class TimMultiplePwmOutputChannel {
 public:
  static_assert(Pins::Count == PMs::Count);

  static constexpr auto NChs = Pins::Count;

  static constexpr auto Channel = Ch0;
  static constexpr auto IsPwm   = true;
  static constexpr auto UsesDma = true;

  template <TimId Tim>
  bool Configure(hal::Dma auto&                        dma,
                 [[maybe_unused]] hstd::Frequency auto f_src) noexcept
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
    EnableTimInterrupt(Tim, 0);

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
        .OCMode       = detail::HalOcMode(PM),
        .Pulse        = 0,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCFastMode   = TIM_OCFAST_DISABLE,
        .OCIdleState  = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
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

}   // namespace stm32g0