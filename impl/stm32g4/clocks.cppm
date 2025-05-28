module;

#include <chrono>
#include <utility>

#include <stm32g4xx_hal.h>

export module hal.stm32g4:clocks;

import hal.abstract;
import hstd;

import :peripherals;

namespace stm32g4 {

using namespace hstd::literals;

export enum class PllSource {
  Hsi = RCC_PLLSOURCE_HSI,
  Hse = RCC_PLLSOURCE_HSE
};

export struct PllSettings {
  bool      enable = true;
  PllSource source = PllSource::Hsi;

  uint32_t m = 4;
  uint32_t n = 85;

  uint32_t p = 2;
  uint32_t q = 2;
  uint32_t r = 2;
};

export enum class SysClkSource {
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK
};

export struct MainClockSettings {
  SysClkSource sys_clk_source         = SysClkSource::Pll;
  uint32_t     ahb_prescaler          = 1;
  uint32_t     apb1_prescaler         = 1;
  uint32_t     apb2_prescaler         = 1;
  uint32_t     system_timer_prescaler = 1;
};

export enum class I2cSourceClock { Pclk1, SysClk, Hsi };

export struct PeripheralSourceClockSettings {
  I2cSourceClock i2c1 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c2 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c3 = I2cSourceClock::Pclk1;
  I2cSourceClock i2c4 = I2cSourceClock::Pclk1;
};

constexpr uint32_t GetHalAhbDivider(uint32_t div) noexcept {
  switch (div) {
  case 1: return RCC_SYSCLK_DIV1;
  case 2: return RCC_SYSCLK_DIV2;
  case 4: return RCC_SYSCLK_DIV4;
  case 8: return RCC_SYSCLK_DIV8;
  case 16: return RCC_SYSCLK_DIV16;
  case 64: return RCC_SYSCLK_DIV64;
  case 128: return RCC_SYSCLK_DIV128;
  case 256: return RCC_SYSCLK_DIV256;
  case 512: return RCC_SYSCLK_DIV512;
  default: std::unreachable();
  }
}

constexpr uint32_t GetHalApbDivider(uint32_t div) noexcept {
  switch (div) {
  case 1: return RCC_HCLK_DIV1;
  case 2: return RCC_HCLK_DIV2;
  case 4: return RCC_HCLK_DIV4;
  case 8: return RCC_HCLK_DIV8;
  case 16: return RCC_HCLK_DIV16;
  default: std::unreachable();
  }
}

constexpr uint32_t GetHalI2c1ClkSource(I2cSourceClock src) noexcept {
  switch (src) {
  case I2cSourceClock::Pclk1: return RCC_I2C1CLKSOURCE_PCLK1;
  case I2cSourceClock::SysClk: return RCC_I2C1CLKSOURCE_SYSCLK;
  case I2cSourceClock::Hsi: return RCC_I2C1CLKSOURCE_HSI;
  default: std::unreachable();
  }
}

constexpr uint32_t GetHalI2c2ClkSource(I2cSourceClock src) noexcept {
  switch (src) {
  case I2cSourceClock::Pclk1: return RCC_I2C2CLKSOURCE_PCLK1;
  case I2cSourceClock::SysClk: return RCC_I2C2CLKSOURCE_SYSCLK;
  case I2cSourceClock::Hsi: return RCC_I2C2CLKSOURCE_HSI;
  default: std::unreachable();
  }
}

constexpr uint32_t GetHalI2c3ClkSource(I2cSourceClock src) noexcept {
  switch (src) {
  case I2cSourceClock::Pclk1: return RCC_I2C3CLKSOURCE_PCLK1;
  case I2cSourceClock::SysClk: return RCC_I2C3CLKSOURCE_SYSCLK;
  case I2cSourceClock::Hsi: return RCC_I2C3CLKSOURCE_HSI;
  default: std::unreachable();
  }
}

constexpr uint32_t GetHalI2c4ClkSource(I2cSourceClock src) noexcept {
  switch (src) {
  case I2cSourceClock::Pclk1: return RCC_I2C4CLKSOURCE_PCLK1;
  case I2cSourceClock::SysClk: return RCC_I2C4CLKSOURCE_SYSCLK;
  case I2cSourceClock::Hsi: return RCC_I2C4CLKSOURCE_HSI;
  default: std::unreachable();
  }
}

export class ClockConfig {
 public:
  static constexpr auto LseFreq = 32'768_Hz;
  static constexpr auto LsiFreq = 32_kHz;

  static constexpr auto HsiFreq = 16_MHz;

  static constexpr auto SysTickFrequency = 1_kHz;

  [[nodiscard]] consteval auto PllClkFreq() const noexcept {
    hstd::hertz result{0};
    if (pll.source == PllSource::Hsi) {
      result = HsiFreq.As<hstd::Hz>();
    } else {
      result = f_hse.As<hstd::Hz>();
    }

    result *= pll.n;
    result /= pll.m;
    result /= pll.r;

    return result;
  }

  [[nodiscard]] consteval auto SysClkFreq() const noexcept {
    switch (mcs.sys_clk_source) {
    case SysClkSource::Hsi: return HsiFreq.As<hstd::Hz>();
    case SysClkSource::Hse: return f_hse.As<hstd::Hz>();
    case SysClkSource::Pll: return PllClkFreq().As<hstd::Hz>();
    default: std::unreachable();
    }
  }

  [[nodiscard]] consteval auto HclkFreq() const noexcept {
    return SysClkFreq() / mcs.ahb_prescaler;
  }

  [[nodiscard]] consteval auto Pclk1Freq() const noexcept {
    return HclkFreq() / mcs.apb1_prescaler;
  }

  [[nodiscard]] consteval auto Pclk2Freq() const noexcept {
    return HclkFreq() / mcs.apb2_prescaler;
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
    case I2cSourceClock::Pclk1: return Pclk1Freq().As<hstd::Hz>();
    case I2cSourceClock::SysClk: return SysClkFreq().As<hstd::Hz>();
    case I2cSourceClock::Hsi: return HsiFreq.As<hstd::Hz>();
    }
  }

  [[nodiscard]] consteval auto PeripheralClkFreq(SpiId id) const {
    switch (id) {
    case SpiId::Spi1: [[fallthrough]];
    case SpiId::Spi4: return Pclk2Freq().As<hstd::Hz>();
    case SpiId::Spi2: [[fallthrough]];
    case SpiId::Spi3: return Pclk1Freq().As<hstd::Hz>();
    default: std::unreachable();
    }
  }

  consteval ClockConfig(hstd::Frequency auto f_hse, PllSettings pll,
                        MainClockSettings             mcs,
                        PeripheralSourceClockSettings pscs = {}) noexcept
      : f_hse{f_hse.template As<hstd::Hz>()}
      , pll{pll}
      , pscs{pscs} {
    // Validate PLL settings
    if (pll.enable) {
      hstd::Assert(pll.m >= 1 && pll.m <= 16, "PLLM must be between 1 and 16");
      hstd::Assert(pll.n >= 8 && pll.n <= 127,
                   "PLLN must be between 8 and 127");

      hstd::Assert(pll.p >= 2 && pll.p <= 31, "PLLP must be between 1 and 31");
      hstd::Assert(pll.q == 2 || pll.q == 4 || pll.q == 6 || pll.q == 8,
                   "PLLQ must be one of 2, 4, 6, 8");
      hstd::Assert(pll.r == 2 || pll.r == 4 || pll.r == 6 || pll.r == 8,
                   "PLLQ must be one of 2, 4, 6, 8");

      hstd::Assert(PllClkFreq().As<hstd::MHz>().count <= 170,
                   "PLLCLK may not exceed 170 MHz");
    }

    // Validate main clock settings
    hstd::Assert(
        hstd::IsPowerOf2(mcs.ahb_prescaler) && mcs.ahb_prescaler <= 512
            && mcs.ahb_prescaler != 32,
        "AHB prescaler must be a power of 2 of at most 512, except 32");
    hstd::Assert(hstd::IsPowerOf2(mcs.apb1_prescaler)
                     && mcs.apb1_prescaler <= 16,
                 "APB1 prescaler must be a power of 2 of at most 16");
    hstd::Assert(hstd::IsPowerOf2(mcs.apb2_prescaler)
                     && mcs.apb2_prescaler <= 16,
                 "APB2 prescaler must be a power of 2 of at most 16");
    hstd::Assert(mcs.system_timer_prescaler == 1
                     || mcs.system_timer_prescaler == 8,
                 "System timer prescaler must be 1 or 8");

    hstd::Assert(Pclk1Freq().As<hstd::MHz>().count <= 170,
                 "PCLK1 may not exceed 170 MHz");
    hstd::Assert(Pclk2Freq().As<hstd::MHz>().count <= 170,
                 "PCLK2 may not exceed 170 MHz");
  }

  [[nodiscard]] bool Configure() const noexcept {
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    RCC_OscInitTypeDef rcc_osc_init = {
        .OscillatorType      = RCC_OSCILLATORTYPE_HSI,
        .HSIState            = RCC_HSI_ON,
        .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
    };

    if (pll.enable) {
      rcc_osc_init.PLL = {
          .PLLState  = RCC_PLL_ON,
          .PLLSource = static_cast<uint32_t>(pll.source),
          .PLLM      = pll.m,
          .PLLN      = pll.n,
          .PLLP      = pll.p,
          .PLLQ      = pll.q,
          .PLLR      = pll.r,
      };
    }

    if (HAL_RCC_OscConfig(&rcc_osc_init) != HAL_OK) {
      return false;
    }

    RCC_ClkInitTypeDef rcc_clk_init = {
        .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
        .SYSCLKSource   = static_cast<uint32_t>(mcs.sys_clk_source),
        .AHBCLKDivider  = GetHalAhbDivider(mcs.ahb_prescaler),
        .APB1CLKDivider = GetHalApbDivider(mcs.apb1_prescaler),
        .APB2CLKDivider = GetHalApbDivider(mcs.apb2_prescaler),
    };
    if (HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_4) != HAL_OK) {
      return false;
    }

    RCC_PeriphCLKInitTypeDef rcc_periph_init{
        .PeriphClockSelection = RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_I2C2
                                | RCC_PERIPHCLK_I2C3 | RCC_PERIPHCLK_I2C4,
        .I2c1ClockSelection = GetHalI2c1ClkSource(pscs.i2c1),
        .I2c2ClockSelection = GetHalI2c2ClkSource(pscs.i2c2),
        .I2c3ClockSelection = GetHalI2c3ClkSource(pscs.i2c3),
        .I2c4ClockSelection = GetHalI2c4ClkSource(pscs.i2c4),
    };

    if (HAL_RCCEx_PeriphCLKConfig(&rcc_periph_init) != HAL_OK) {
      return false;
    }

    return true;
  }

  hstd::hertz                   f_hse;
  PllSettings                   pll;
  MainClockSettings             mcs;
  PeripheralSourceClockSettings pscs;
};

export template <typename C>
concept ClockFrequencies = hal::ClockFrequencies<C> && requires(const C& cfs) {
  { cfs.PeripheralClkFreq(std::declval<I2cId>()) } -> hstd::Frequency;
  { cfs.PeripheralClkFreq(std::declval<SpiId>()) } -> hstd::Frequency;
};

static_assert(ClockFrequencies<ClockConfig>);

export class SysTickClock {
public:
  using rep        = uint32_t;
  using period     = std::milli;
  using duration   = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<SysTickClock, duration>;

  static constexpr auto is_steady = false;

  [[nodiscard]] static time_point now() noexcept {
    return time_point{duration{HAL_GetTick()}};
  }

  static void BlockFor(hstd::Duration auto duration) noexcept {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    HAL_Delay(ms.count());
  }
};

}   // namespace stm32g4