module;

#include <span>
#include <type_traits>
#include <utility>

#include <stm32g4xx_hal.h>
#include <stm32g4xx_hal_tim_ex.h>

#include <pin_macros.h>

export module hal.stm32g4:tim.channel;

import hstd;

import :pin;
import :tim.common;
import :tim.features;
import :pin_mapping.tim;

namespace stm32g4 {

/**
 * Concept describing a timer channel
 */
template <typename Impl>
concept TimChannel = requires(Impl impl) {
  // Channel number
  { Impl::Channel } -> std::convertible_to<unsigned>;

  // Whether the channel is a PWM channel
  { Impl::IsPwm } -> std::convertible_to<bool>;

  // Whether the channel uses DMA
  { Impl::UsesDma } -> std::convertible_to<bool>;

  // Whether the channel uses the timer interrupt
  { Impl::UsesInterrupt } -> std::convertible_to<bool>;
};

/**
 * Modes for controlling PWM on a timer channel
 */
export enum class PwmControlMode {
  Direct,   //!< Control PWM by writing to the compare register
  Dma,      //!< Control PWM through DMA
};

/**
 * Possible PWM modes, for details refer to STM32 datasheet
 */
export enum class PwmMode { Pwm1, Pwm2 };

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
  if (ch <= 6) {
    return (ch - 1) * 4;
  }
  std::unreachable();
}

export template <TimId Id, unsigned Ch,
                 hal::DmaPriority Prio = hal::DmaPriority::Low>
using TimChDma = DmaChannel<Id, ChannelToDmaRequest(Ch), Prio>;

export enum class Polarity { High, Low };

export enum class PwmOutputIdleState { Reset, Set };

export struct PwmOutputChannelSettings {
  PwmControlMode control_mode                = PwmControlMode::Direct;
  PwmMode        pwm_mode                    = PwmMode::Pwm1;
  bool           enable_output               = true;
  bool           enable_complementary_output = false;

  PinId              pin;
  PinId              complementary_pin            = {};
  Polarity           polarity                     = Polarity::High;
  Polarity           complementary_polarity       = Polarity::High;
  PwmOutputIdleState idle_state                   = PwmOutputIdleState::Reset;
  PwmOutputIdleState complementary_pwm_idle_state = PwmOutputIdleState::Reset;

  [[nodiscard]] constexpr TIM_OC_InitTypeDef ToHalInitStruct() const noexcept {
    return {
        .OCMode      = HalOcMode(pwm_mode),
        .Pulse       = 0,
        .OCPolarity  = polarity == Polarity::High ? TIM_OCPOLARITY_HIGH
                                                  : TIM_OCPOLARITY_LOW,
        .OCNPolarity = complementary_polarity == Polarity::High
                           ? TIM_OCNPOLARITY_HIGH
                           : TIM_OCNPOLARITY_LOW,
        .OCFastMode  = TIM_OCFAST_DISABLE,
        .OCIdleState = idle_state == PwmOutputIdleState::Reset
                           ? TIM_OCIDLESTATE_RESET
                           : TIM_OCIDLESTATE_SET,
        .OCNIdleState =
            complementary_pwm_idle_state == PwmOutputIdleState::Reset
                ? TIM_OCNIDLESTATE_RESET
                : TIM_OCNIDLESTATE_SET,
    };
  }
};

export template <unsigned Ch, PwmOutputChannelSettings S>
class TimPwmOutputChannel {
 public:
  static constexpr auto Channel       = Ch;
  static constexpr auto IsPwm         = true;
  static constexpr auto UsesDma       = S.control_mode == PwmControlMode::Dma;
  static constexpr auto UsesInterrupt = true;

  static constexpr auto ControlMode = S.control_mode;
  static constexpr auto PwmMode     = S.pwm_mode;

  /**
   * Configures the PWM output channel
   * @tparam Tim Timer ID
   * @param f_src Source clock frequency
   * @return Whether configuration was successful
   */
  template <TimId Tim>
  bool Configure([[maybe_unused]] hstd::Frequency auto f_src) noexcept {
    static_assert(TimHasChannel(Tim, Ch), "Timer channel does not exist");
    static_assert(TimChIsPwmOutputCapable(Tim, Ch),
                  "Timer channel is not capable of generating PWM with output");

    // Configure channel
    auto init = S.ToHalInitStruct();
    if (HAL_TIM_PWM_ConfigChannel(&htim, &init, HalChannel(Ch)) != HAL_OK) {
      return false;
    }

    // Configure pin
    if constexpr (S.enable_output) {
      constexpr auto MappingOpt =
          hal::FindTimAFMapping(TimChPinMappings, Tim, Ch, S.pin);
      static_assert(MappingOpt.has_value(),
                    "Pin is not valid for this timer channel");
      constexpr auto Mapping = *MappingOpt;

      Pin::InitializeAlternate(Mapping.pin, Mapping.af);
    }

    if constexpr (S.enable_complementary_output) {
      constexpr auto MappingOpt = hal::FindTimAFMapping(
          TimChNPinMappings, Tim, Ch, S.complementary_pin);
      static_assert(MappingOpt.has_value(),
                    "Pin is not valid for this timer channel");
      constexpr auto Mapping = *MappingOpt;

      Pin::InitializeAlternate(Mapping.pin, Mapping.af);
    }

    return true;
  }

  template <TimId Tim>
  bool Configure(hal::Dma auto&                        dma,
                 [[maybe_unused]] hstd::Frequency auto f_src) noexcept
    requires UsesDma
  {
    using DmaCh = TimUpdateDma<Tim>;

    // Call base Configure
    if (!Configure<Tim>(f_src)) {
      return false;
    }

    // Configure DMA
    auto& hdma = dma.template SetupChannel<DmaCh>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
        hal::DmaDataWidth::HalfWord, false, hal::DmaDataWidth::HalfWord, true);
    __HAL_LINKDMA(&htim, hdma[TIM_DMA_ID_UPDATE], hdma);

    return true;
  }

  void Enable() {
    if constexpr (S.enable_output) {
      constexpr auto EnableBit = 0b1UL << HalChannel(Ch);
      htim.Instance->CCER |= EnableBit;
    }

    if constexpr (S.enable_complementary_output) {
      constexpr auto EnableBit = 0b100UL << HalChannel(Ch);
      htim.Instance->CCER |= EnableBit;
    }
  }

  void Disable() {
    if constexpr (S.enable_output) {
      constexpr auto EnableBit = 0b1UL << HalChannel(Ch);
      htim.Instance->CCER &= ~EnableBit;
    }

    if constexpr (S.enable_complementary_output) {
      constexpr auto EnableBit = 0b100UL << HalChannel(Ch);
      htim.Instance->CCER &= ~EnableBit;
    }
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

export template <std::size_t N>
struct BurstDmaChannelsSettings {
  static constexpr auto NumChannels = N;

  unsigned             first_channel;
  std::array<PinId, N> pins;

  std::array<PwmMode, N> pwm_modes = hstd::FillArray<PwmMode, N>(PwmMode::Pwm1);

  bool include_repetition_counter = false;
};

export template <BurstDmaChannelsSettings S>
class TimBurstDmaOutputChannels {
 public:
  static constexpr auto Channel       = S.first_channel;
  static constexpr auto IsPwm         = true;
  static constexpr auto UsesDma       = true;
  static constexpr auto UsesInterrupt = true;

  static_assert(hstd::Implies(S.include_repetition_counter,
                              S.first_channel == 1),
                "Repetition counter can only be included in burst DMA if first "
                "channel is channel 1");

  template <TimId Tim>
  bool Configure(hal::Dma auto&                        dma,
                 [[maybe_unused]] hstd::Frequency auto f_src) noexcept
    requires UsesDma
  {
    // Configure channels
    if (!ConfigureChannels<Tim>(std::make_index_sequence<S.NumChannels>())) {
      return false;
    }

    // Configure DMA
    using DmaCh = TimUpdateDma<Tim>;

    auto& hdma = dma.template SetupChannel<DmaCh>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
        hal::DmaDataWidth::Word, false, hal::DmaDataWidth::Word, true);
    __HAL_LINKDMA(&htim, hdma[TIM_DMA_ID_UPDATE], hdma);

    return true;
  }

  void EnableCaptureComparePreload() {
    // For channel 1, 2
    constexpr auto FirstChannel = S.first_channel;
    constexpr auto LastChannel  = S.first_channel + (S.NumChannels - 1);

    constexpr auto CreateBitMask = [](bool ch1, bool ch2) {
      uint32_t result = 0;
      if (ch1) {
        result |= (0b1UL << 3UL);
      }

      if (ch2) {
        result |= (0b1UL << 11UL);
      }

      return result;
    };

    if constexpr (FirstChannel <= 2) {
      constexpr auto Bits = CreateBitMask(
          FirstChannel == 1, FirstChannel <= 2 && LastChannel >= 2);
      htim.Instance->CCMR1 |= Bits;
    }

    if constexpr (LastChannel >= 3) {
      constexpr auto Bits =
          CreateBitMask(FirstChannel <= 3 && LastChannel >= 3,
                        FirstChannel <= 4 && LastChannel >= 4);
      htim.Instance->CCMR2 |= Bits;
    }
  }

  void Enable() {
    constexpr auto EnableBits = ([](unsigned first_chan, std::size_t n_chans) {
      uint32_t result = 0;
      for (std::size_t i = 0; i < n_chans; ++i) {
        result |= (0b1U << HalChannel(first_chan + i));
      }
      return result;
    })(S.first_channel, S.NumChannels);

    htim.Instance->CCER |= EnableBits;
  }

  void Disable() {
    for (auto i = 0; i < S.num_cannels; i++) {
      TIM_CCxChannelCmd(htim.Instance, HalChannel(S.first_channel + i),
                        TIM_CCx_DISABLE);
    }
  }

  bool SetDmaData(std::span<const uint32_t> cmps) {
    if constexpr (S.include_repetition_counter) {
      return HAL_TIM_DMABurst_MultiWriteStart(&htim, TIM_DMABASE_RCR,
                                              TIM_DMA_UPDATE, (cmps.data()),
                                              (S.NumChannels) << 8, cmps.size())
             == HAL_OK;
    } else {
      return HAL_TIM_DMABurst_MultiWriteStart(
                 &htim, TIM_DMABASE_CCR1 + (S.first_channel - 1),
                 TIM_DMA_UPDATE, (cmps.data()), (S.NumChannels - 1) << 8,
                 cmps.size())
             == HAL_OK;
    }
  }

  void ChannelPeriodElapsedCallback() {
    HAL_TIM_DMABurst_WriteStop(&htim, TIM_DMA_UPDATE);
  }

 protected:
  explicit TimBurstDmaOutputChannels(TIM_HandleTypeDef& htim)
      : htim{htim} {}

 private:
  template <TimId Tim, std::size_t... Idxs>
  bool ConfigureChannels(std::index_sequence<Idxs...>) {
    return (... && ConfigureChannel<Tim, Idxs>());
  }

  template <TimId Tim, std::size_t I>
  bool ConfigureChannel() {
    constexpr auto Ch = S.first_channel + I;

    static_assert(TimHasChannel(Tim, Ch), "Timer channel does not exist");
    static_assert(TimChIsPwmOutputCapable(Tim, Ch),
                  "Timer channel is not capable of generating PWM with output");

    constexpr PinId   P  = S.pins[I];
    constexpr PwmMode PM = S.pwm_modes[I];

    // Configure channel
    TIM_OC_InitTypeDef init{
        .OCMode       = HalOcMode(PM),
        .Pulse        = 0,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCFastMode   = TIM_OCFAST_DISABLE,
        .OCIdleState  = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
    };

    if (HAL_TIM_OC_ConfigChannel(&htim, &init, HalChannel(Ch)) != HAL_OK) {
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
  void SetComparesImpl(std::array<uint16_t, S.NumChannels> new_cmps,
                       std::index_sequence<Idxs...>) {
    (..., SetCompare<Idxs>(new_cmps[Idxs]));
  }

  template <unsigned Idx>
  void SetCompare(uint32_t new_cmp) noexcept {
    constexpr auto Ch = S.first_channel + Idx;
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

}   // namespace stm32g4