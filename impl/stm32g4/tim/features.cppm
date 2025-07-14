module;

#include <cstdint>
#include <utility>

export module hal.stm32g4:tim.features;

import :peripherals;

namespace stm32g4 {

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
  case Tim15: return DualChannelWithComplementary;
  case Tim16: return SingleChannelWithComplementary;
  case Tim17: return SingleChannelWithComplementary;
  case Tim20: return Advanced;
  }

  std::unreachable();
}

export constexpr bool TimHasChannel(TimId tim, unsigned chan) noexcept {
  using enum TimType;

  switch (GetTimerType(tim)) {
  case Advanced: return chan >= 1 && chan <= 6;
  case GeneralPurpose32Bit: [[fallthrough]];
  case GeneralPurpose16Bit: return chan >= 1 && chan <= 4;
  case Basic: return false;
  case SingleChannel: return chan == 1;
  case DualChannel: return chan == 1 || chan == 2;
  case SingleChannelWithComplementary: return chan == 1;
  case DualChannelWithComplementary: return chan == 1 || chan == 2;
  }

  std::unreachable();
}

export constexpr bool TimChIsPwmOutputCapable(TimId    tim,
                                              unsigned chan) noexcept {
  using enum TimType;

  switch (GetTimerType(tim)) {
  case Advanced: return chan >= 1 && chan <= 4;
  case GeneralPurpose32Bit: [[fallthrough]];
  case GeneralPurpose16Bit: return chan >= 1 && chan <= 4;
  case Basic: return false;
  case SingleChannel: return chan == 1;
  case DualChannel: return chan == 1 || chan == 2;
  case SingleChannelWithComplementary: return chan == 1;
  case DualChannelWithComplementary: return chan == 1 || chan == 2;
  }

  std::unreachable();
}

export constexpr bool
TimChIsPwmComplementaryOutputCapable(TimId tim, unsigned chan) noexcept {
  using enum TimType;

  switch (GetTimerType(tim)) {
  case Advanced: return chan >= 1 && chan <= 4;
  case SingleChannelWithComplementary: [[fallthrough]];
  case DualChannelWithComplementary: return chan == 1;
  default: return false;
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

export [[nodiscard]] constexpr uint32_t MaxTimerPeriod(TimId tim) noexcept {
  switch (BitCount(tim)) {
  case TimerBitCount::Bits16: return 0xFFFFU;
  case TimerBitCount::Bits32: return 0xFFFF'FFFF;
  default: std::unreachable();
  }
}

}   // namespace stm32g4
