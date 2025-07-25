#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <stm32g4xx_hal.h>

#include <hal/clocks.h>

#include <constexpr_tools/helpers.h>
#include <constexpr_tools/math.h>
#include <halstd/chrono_ex.h>

#include <stm32g4_old/peripheral_ids.h>

namespace stm32g4 {

using namespace halstd::literals;

enum class PllSource { Hsi = RCC_PLLSOURCE_HSI, Hse = RCC_PLLSOURCE_HSE };

struct PllSettings {
  bool      enable = true;
  PllSource source = PllSource::Hsi;

  uint32_t m = 4;
  uint32_t n = 85;

  uint32_t p = 2;
  uint32_t q = 2;
  uint32_t r = 2;
};

enum class SysClkSource {
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK
};

struct SystemClockSettings {
  SysClkSource sys_clk_source         = SysClkSource::Pll;
  uint32_t     ahb_prescaler          = 1;
  uint32_t     apb1_prescaler         = 1;
  uint32_t     apb2_prescaler         = 1;
  uint32_t     system_timer_prescaler = 1;
};

enum class I2cSourceClock { Pclk1, SysClk, Hsi };

struct PeripheralSourceClockSettings {
  I2cSourceClock i2c1 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c2 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c3 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c4 = I2cSourceClock::Pclk1;
};

class ClockSettings {
 public:
  static constexpr auto LseFreq = 32'768_Hz;
  static constexpr auto LsiFreq = 32_kHz;

  static constexpr auto HsiFreq = 16_MHz;

  static constexpr auto SysTickFrequency = 1_kHz;

  [[nodiscard]] consteval auto PllClkFreq() const noexcept {
    halstd::hertz result{0};
    if (pll.source == PllSource::Hsi) {
      result = HsiFreq.As<halstd::Hz>();
    } else {
      result = f_hse.As<halstd::Hz>();
    }

    result *= pll.n;
    result /= pll.m;
    result /= pll.r;

    return result;
  }

  [[nodiscard]] consteval auto SysClkSourceClockFrequency() const noexcept {
    switch (system_clock_settings.sys_clk_source) {
    case SysClkSource::Hsi: return HsiFreq.As<halstd::Hz>();
    case SysClkSource::Hse: return f_hse.As<halstd::Hz>();
    case SysClkSource::Pll: return PllClkFreq().As<halstd::Hz>();
    default: std::unreachable();
    }
  }

  [[nodiscard]] consteval auto HclkFreq() const noexcept {
    return SysClkSourceClockFrequency() / system_clock_settings.ahb_prescaler;
  }

  [[nodiscard]] consteval auto Pclk1Freq() const noexcept {
    return HclkFreq() / system_clock_settings.apb1_prescaler;
  }

  [[nodiscard]] consteval auto Pclk2Freq() const noexcept {
    return HclkFreq() / system_clock_settings.apb2_prescaler;
  }

  [[nodiscard]] consteval auto PeripheralClkFreq(I2cId id) const {
    I2cSourceClock clk_src{};

    switch (id) {
    case I2cId::I2c1: clk_src = pscs.i2c1; break;
    case I2cId::I2c2: clk_src = pscs.i2c2; break;
    case I2cId::I2c3: clk_src = pscs.i2c3; break;
    case I2cId::I2c4: clk_src = pscs.i2c4; break;
    default: std::unreachable();
    }

    switch (clk_src) {
    case I2cSourceClock::Pclk1: return Pclk1Freq().As<halstd::Hz>();
    case I2cSourceClock::SysClk: return SysClkSourceClockFrequency().As<halstd::Hz>();
    case I2cSourceClock::Hsi: return HsiFreq.As<halstd::Hz>();
    }
  }

  [[nodiscard]] consteval auto PeripheralClkFreq(SpiId id) const {
    switch (id) {
    case SpiId::Spi1: [[fallthrough]];
    case SpiId::Spi4: return Pclk2Freq().As<halstd::Hz>();
    case SpiId::Spi2: [[fallthrough]];
    case SpiId::Spi3: return Pclk1Freq().As<halstd::Hz>();
    default: std::unreachable();
    }
  }

  consteval ClockSettings(halstd::Frequency auto f_hse, PllSettings pll,
                        SystemClockSettings             mcs,
                        PeripheralSourceClockSettings pscs = {}) noexcept
      : f_hse{f_hse.template As<halstd::Hz>()}
      , pll{pll}
      , pscs{pscs} {
    // Validate PLL settings
    if (pll.enable) {
      assert(("PLLM must be between 1 and 16", (pll.m >= 1 && pll.m <= 16)));
      assert(("PLLN must be between 8 and 127", (pll.n >= 8 && pll.n <= 127)));

      assert(("PLLP must be between 1 and 31", (pll.p >= 2 && pll.p <= 31)));
      assert(("PLLQ must be one of 2, 4, 6, 8",
              (pll.q == 2 || pll.q == 4 || pll.q == 6 || pll.q == 8)));
      assert(("PLLQ must be one of 2, 4, 6, 8",
              (pll.r == 2 || pll.r == 4 || pll.r == 6 || pll.r == 8)));

      assert(("PLLCLK may not exceed 170 MHz",
              (PllClkFreq().As<halstd::MHz>().count <= 170)));
    }

    // Validate main clock settings
    assert(("AHB prescaler must be a power of 2 of at most 512, except 32",
            (ct::IsPowerOf2(mcs.ahb_prescaler) && mcs.ahb_prescaler <= 512
             && mcs.ahb_prescaler != 32)));
    assert(("APB1 prescaler must be a power of 2 of at most 16",
            (ct::IsPowerOf2(mcs.apb1_prescaler) && mcs.apb1_prescaler <= 16)));
    assert(("APB2 prescaler must be a power of 2 of at most 16",
            (ct::IsPowerOf2(mcs.apb2_prescaler) && mcs.apb2_prescaler <= 16)));
    assert(
        ("System timer prescaler must be 1 or 8",
         (mcs.system_timer_prescaler == 1 || mcs.system_timer_prescaler == 8)));

    assert(("PCLK1 may not exceed 170 MHz",
            (Pclk1Freq().As<halstd::MHz>().count <= 170)));
    assert(("PCLK2 may not exceed 170 MHz",
            (Pclk2Freq().As<halstd::MHz>().count <= 170)));
  }

  [[nodiscard]] bool Configure() const noexcept;

  halstd::hertz                 f_hse;
  PllSettings                   pll;
  SystemClockSettings             system_clock_settings;
  PeripheralSourceClockSettings pscs;
};

template <typename C>
concept ClockFrequencies = hal::ClockFrequencies<C> && requires(const C& cfs) {
  { cfs.PeripheralClkFreq(std::declval<I2cId>()) } -> halstd::Frequency;
  { cfs.PeripheralClkFreq(std::declval<SpiId>()) } -> halstd::Frequency;
};

static_assert(ClockFrequencies<ClockSettings>);

template <hal::ClockFrequencies auto CF>
class SysTickClock {
 public:
  constexpr static halstd::Duration auto TimeSinceBoot() noexcept {
    constexpr auto systick_freq = CF.SysTickFrequency;

    return HAL_GetTick() * systick_freq.Period();
  }
};

}   // namespace stm32g4