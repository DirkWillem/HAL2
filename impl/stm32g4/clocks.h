#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <stm32g4xx_hal.h>

#include <constexpr_tools/helpers.h>
#include <constexpr_tools/math.h>

#include <stm32g4/peripheral_ids.h>

namespace stm32g4 {

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

struct MainClockSettings {
  SysClkSource sys_clk_source         = SysClkSource::Pll;
  uint32_t     ahb_prescaler          = 1;
  uint32_t     apb1_prescaler         = 8;
  uint32_t     apb2_prescaler         = 2;
  uint32_t     system_timer_prescaler = 1;
};

enum class I2cSourceClock { Pclk1, SysClk, Hsi };

struct PeripheralSourceClockSettings {
  I2cSourceClock i2c1 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c2 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c3 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c4 = I2cSourceClock::Pclk1;
};

class ClockConfig {
 public:
  static constexpr uint32_t LseFreq = 32'768;
  static constexpr uint32_t LsiFreq = 32'000;

  static constexpr uint32_t HsiFreq = 16'000'000;

  consteval ClockConfig(uint32_t f_hse, PllSettings pll, MainClockSettings mcs,
                        PeripheralSourceClockSettings pscs = {}) noexcept
      : f_hse{f_hse}
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

      assert(("PLLCLK may not exceed 170 MHz", (PllClkFreq() <= 170'000'000)));
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

    assert(("PCLK1 may not exceed 170 MHz", (Pclk1Freq() <= 170'000'000)));
    assert(("PCLK2 may not exceed 170 MHz", (Pclk2Freq() <= 170'000'000)));
  }

  [[nodiscard]] consteval uint32_t PllClkFreq() const noexcept {
    uint64_t result = 0;
    if (pll.source == PllSource::Hsi) {
      result = HsiFreq;
    } else {
      result = f_hse;
    }

    result *= pll.n;
    result /= pll.m;
    result /= pll.r;

    if (result > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) {
      return std::numeric_limits<uint32_t>::max();
    } else {
      return static_cast<uint32_t>(result);
    }
  }

  [[nodiscard]] consteval uint32_t SysClkFreq() const noexcept {
    switch (mcs.sys_clk_source) {
    case SysClkSource::Hsi: return HsiFreq;
    case SysClkSource::Hse: return f_hse;
    case SysClkSource::Pll: return PllClkFreq();
    default: std::unreachable();
    }
  }

  [[nodiscard]] consteval uint32_t HclkFreq() const noexcept {
    return SysClkFreq() / mcs.ahb_prescaler;
  }

  [[nodiscard]] consteval uint32_t Pclk1Freq() const noexcept {
    return HclkFreq() / mcs.apb1_prescaler;
  }

  [[nodiscard]] consteval uint32_t Pclk2Freq() const noexcept {
    return HclkFreq() / mcs.apb2_prescaler;
  }

  [[nodiscard]] consteval uint32_t PeripheralClkFreq(I2cId id) const {
    I2cSourceClock clk_src{};

    switch (id) {
    case I2cId::I2c1: clk_src = pscs.i2c1; break;
    case I2cId::I2c2: clk_src = pscs.i2c2; break;
    case I2cId::I2c3: clk_src = pscs.i2c3; break;
    case I2cId::I2c4: clk_src = pscs.i2c4; break;
    default: std::unreachable();
    }

    switch (clk_src) {
    case I2cSourceClock::Pclk1: return Pclk1Freq();
    case I2cSourceClock::SysClk: return SysClkFreq();
    case I2cSourceClock::Hsi: return HsiFreq;
    }
  }

  [[nodiscard]] bool Configure() const noexcept;

  uint32_t                      f_hse;
  PllSettings                   pll;
  MainClockSettings             mcs;
  PeripheralSourceClockSettings pscs;
};

template <typename C>
concept ClockFrequencies = requires(const C& cfs) {
  {
    cfs.PeripheralClkFreq(std::declval<I2cId>())
  } -> std::convertible_to<uint32_t>;
};

static_assert(ClockFrequencies<ClockConfig>);

}   // namespace stm32g4