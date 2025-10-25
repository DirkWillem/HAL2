#pragma once

#include <cassert>

#include <constexpr_tools/chrono_ex.h>
#include <constexpr_tools/logic.h>
#include <constexpr_tools/static_mapping.h>

#include <stm32h5xx_hal.h>
#include <stm32h5xx_ll_rcc.h>

namespace stm32h5 {

using namespace ct::literals;

inline constexpr auto HsiFrequency   = 64_MHz;
inline constexpr auto Hsi48Frequency = 48_MHz;
inline constexpr auto CsiFrequency   = 4_MHz;

enum class PllSource : uint32_t {
  Hsi = RCC_PLLSOURCE_HSI,
  Csi = RCC_PLLSOURCE_CSI,
  Hse = RCC_PLLSOURCE_HSE
};

struct PllSettings {
  bool enable;

  uint32_t m;
  uint32_t n;

  uint32_t p;
  uint32_t q;
  uint32_t r;

  /**
   * Calculates the PLL input frequency (VCI / refx_ck) given the source clock
   * frequency
   * @param src Source clock frequency
   * @return refx_ck
   */
  [[nodiscard]] consteval ct::Frequency auto
  PllInputFrequency(ct::Frequency auto src) const noexcept {
    ct::hertz result = src.template As<ct::Hz>();
    result /= m;
    return result;
  }

  /**
   * Returns the PLL output frequency (VCO / vcox_ck) given the source clock
   * @param src Source clock frequency
   * @return vcox_ck
   */
  [[nodiscard]] consteval ct::Frequency auto
  PllOutputFrequency(ct::Frequency auto src) const noexcept {
    ct::hertz result = src.template As<ct::Hz>();
    result /= m;
    result *= n;
    return result;
  }

  [[nodiscard]] consteval ct::Frequency auto
  OutputP(ct::Frequency auto src) const noexcept {
    return Output(src, p);
  }

  [[nodiscard]] consteval ct::Frequency auto
  OutputQ(ct::Frequency auto src) const noexcept {
    return Output(src, q);
  }

  [[nodiscard]] consteval ct::Frequency auto
  OutputR(ct::Frequency auto src) const noexcept {
    return Output(src, r);
  }

 private:
  [[nodiscard]] constexpr ct::Frequency auto
  Output(ct::Frequency auto src, uint32_t div_pqr) const noexcept {
    ct::hertz result = src.template As<ct::Hz>();
    result /= m;
    result *= n;
    result /= div_pqr;
    return result;
  }
};

struct PllsSettings {
  PllSettings pll1;
  PllSettings pll2;
  PllSettings pll3;
};

enum class SysClkSource : uint32_t {
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Csi = RCC_SYSCLKSOURCE_CSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK
};

struct SystemClockSettings {
  uint32_t ahb_prescaler;
  uint32_t apb1_prescaler;
  uint32_t apb2_prescaler;
  uint32_t apb3_prescaler;

  [[nodiscard]] consteval bool
  Validate(ct::Frequency auto sysclk) const noexcept {
    // Validate D1 prescaler values
    assert(("AHB Prescaler must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(ahb_prescaler)));

    // Validate all D2 prescaler values
    assert(("APB1 Prescaler must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(apb1_prescaler)));
    assert(("APB2 Prescaler must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(apb2_prescaler)));
    assert(("APB3 Prescaler must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(apb3_prescaler)));

    // Validate maximum frequencies
    assert(("AHB Frequency (HCLK) may not exceed 250 MHz",
            AhbClockFrequency(sysclk).template As<ct::Hz>()
                <= (250_MHz).As<ct::Hz>()));

    assert(("APB1 peripherals clock (PCLK1) frequency may not exceed 250 MHz",
            Apb1PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (250_MHz).As<ct::Hz>()));
    assert(("APB2 peripherals clock (PCLK2) frequency may not exceed 250 MHz",
            Apb2PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (250_MHz).As<ct::Hz>()));
    assert(("APB3 peripherals clock (PCLK3) frequency may not exceed 250 MHz",
            Apb3PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (250_MHz).As<ct::Hz>()));

    return true;
  }

  [[nodiscard]] consteval ct::Frequency auto
  AhbClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return sysclk / ahb_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb1PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb1_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb1TimersClockFrequency(ct::Frequency auto sysclk) const noexcept {
    if (apb1_prescaler == 1) {
      return Apb1PeripheralsClockFrequency(sysclk);
    } else {
      return Apb1PeripheralsClockFrequency(sysclk) * 2;
    }
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb2PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb2_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb2TimersClockFrequency(ct::Frequency auto sysclk) const noexcept {
    if (apb2_prescaler == 1) {
      return Apb2PeripheralsClockFrequency(sysclk);
    } else {
      return Apb2PeripheralsClockFrequency(sysclk) * 2;
    }
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb3PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb3_prescaler;
  }
};

inline constexpr PllSource    DefaultPllSource = PllSource::Hsi;
inline constexpr PllsSettings DefaultPllSettings{
    .pll1 = {.enable = true, .m = 16, .n = 125, .p = 2, .q = 2, .r = 2},
    .pll2 = {},
    .pll3 = {},
};

inline constexpr SysClkSource        DefaultSysClkSource = SysClkSource::Pll;
inline constexpr SystemClockSettings DefaultSystemClockSettings = {
    .ahb_prescaler  = 1,
    .apb1_prescaler = 1,
    .apb2_prescaler = 1,
    .apb3_prescaler = 1,
};

struct ClockSettings {
  ct::hertz           f_hse;
  PllSource           pll_source = DefaultPllSource;
  PllsSettings        pll;
  SysClkSource        sysclk_source = DefaultSysClkSource;
  SystemClockSettings system_clock_settings;

  [[nodiscard]] consteval ct::Frequency auto
  PllSourceClockFrequency() const noexcept {
    switch (pll_source) {
    case PllSource::Hsi: return HsiFrequency.As<ct::Hz>();
    case PllSource::Csi: return CsiFrequency.As<ct::Hz>();
    case PllSource::Hse: return f_hse.As<ct::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval ct::Frequency auto
  SysClkSourceClockFrequency() const noexcept {
    switch (sysclk_source) {
    case SysClkSource::Hsi: return HsiFrequency.As<ct::Hz>();
    case SysClkSource::Csi: return CsiFrequency.As<ct::Hz>();
    case SysClkSource::Hse: return f_hse.As<ct::Hz>();
    case SysClkSource::Pll:
      return pll.pll1.OutputP(PllSourceClockFrequency()).As<ct::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval bool Validate() const noexcept {
    // Unimplemented features
    assert(("PLL2/3 configuration is not yet implemented",
            !pll.pll2.enable && !pll.pll3.enable));

    return system_clock_settings.Validate(
        SysClkSourceClockFrequency().As<ct::Hz>());
  }
};

inline constexpr ClockSettings DefaultClockSettings{
    .f_hse                 = (25_MHz).As<ct::Hz>(),
    .pll_source            = PllSource::Hsi,
    .pll                   = DefaultPllSettings,
    .sysclk_source         = SysClkSource::Pll,
    .system_clock_settings = DefaultSystemClockSettings,
};

static_assert(DefaultClockSettings.Validate());

namespace detail {

enum class Pll {
  Pll1,
  Pll2,
  Pll3,
};

enum class PllVciRange {
  Range0 = LL_RCC_PLLINPUTRANGE_1_2,
  Range1 = LL_RCC_PLLINPUTRANGE_2_4,
  Range2 = LL_RCC_PLLINPUTRANGE_4_8,
  Range3 = LL_RCC_PLLINPUTRANGE_8_16,
};

[[nodiscard]] consteval uint32_t
GetShiftedPllVciRange(Pll pll, PllVciRange range) noexcept {
  switch (pll) {
  case Pll::Pll1:
    return static_cast<uint32_t>(range) << RCC_PLL1CFGR_PLL1RGE_Pos;
  case Pll::Pll2:
    return static_cast<uint32_t>(range) << RCC_PLL2CFGR_PLL2RGE_Pos;
  case Pll::Pll3:
    return static_cast<uint32_t>(range) << RCC_PLL3CFGR_PLL3RGE_Pos;
  }

  std::unreachable();
}

[[nodiscard]] consteval PllVciRange
GetPllVciRange(ct::Frequency auto vci_freq) noexcept {
  const auto f_hz = vci_freq.template As<ct::Hz>();

  if (f_hz >= 1_MHz && f_hz < 2_MHz) {
    return PllVciRange::Range0;
  } else if (f_hz <= 4_MHz) {
    return PllVciRange::Range1;
  } else if (f_hz <= 8_MHz) {
    return PllVciRange::Range2;
  } else if (f_hz <= 16_MHz) {
    return PllVciRange::Range3;
  }

  std::unreachable();
}

static_assert(GetShiftedPllVciRange(Pll::Pll1, GetPllVciRange(10_MHz))
              == RCC_PLLVCIRANGE_3);

enum class PllVcoRange {
  Wide   = LL_RCC_PLLVCORANGE_WIDE,
  Medium = LL_RCC_PLLVCORANGE_MEDIUM,
};

[[nodiscard]] consteval PllVcoRange
GetPllVcoRange(ct::Frequency auto vco_freq) noexcept {
  const auto f_hz = vco_freq.template As<ct::Hz>();

  if (f_hz >= 192_MHz && f_hz <= 836_MHz) {
    return PllVcoRange::Wide;
  } else if (f_hz >= 150_MHz && f_hz <= 420_MHz) {
    return PllVcoRange::Medium;
  }

  std::unreachable();
}

[[nodiscard]] consteval uint32_t
GetShiftedPllVcoRange(Pll pll, PllVcoRange range) noexcept {
  switch (pll) {
  case Pll::Pll1:
    return static_cast<uint32_t>(range) << RCC_PLL1CFGR_PLL1VCOSEL_Pos;
  case Pll::Pll2:
    return static_cast<uint32_t>(range) << RCC_PLL2CFGR_PLL2VCOSEL_Pos;
  case Pll::Pll3:
    return static_cast<uint32_t>(range) << RCC_PLL3CFGR_PLL3VCOSEL_Pos;
  }

  std::unreachable();
}

[[nodiscard]] consteval uint32_t GetAhbDivider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 9>(
      div, {{
               std::make_pair(1, RCC_SYSCLK_DIV1),
               std::make_pair(2, RCC_SYSCLK_DIV2),
               std::make_pair(4, RCC_SYSCLK_DIV4),
               std::make_pair(8, RCC_SYSCLK_DIV8),
               std::make_pair(16, RCC_SYSCLK_DIV16),
               std::make_pair(64, RCC_SYSCLK_DIV64),
               std::make_pair(128, RCC_SYSCLK_DIV128),
               std::make_pair(256, RCC_SYSCLK_DIV256),
               std::make_pair(512, RCC_SYSCLK_DIV512),
           }});
}

[[nodiscard]] consteval uint32_t GetApbDivider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 5>(div,
                                         {{
                                             {1, RCC_HCLK_DIV1},
                                             std::make_pair(2, RCC_HCLK_DIV2),
                                             std::make_pair(4, RCC_HCLK_DIV4),
                                             std::make_pair(8, RCC_HCLK_DIV8),
                                             std::make_pair(16, RCC_HCLK_DIV16),
                                         }});
}

[[nodiscard]] consteval uint32_t
GetVosRange(ct::Frequency auto f_hclk) noexcept {
  return PWR_REGULATOR_VOLTAGE_SCALE0;
}

[[nodiscard]] consteval uint32_t GetFlashLatency(ct::Frequency auto f_hclk,
                                                 uint32_t           vos_range) {
  const auto f_hz = f_hclk.template As<ct::Hz>();

  switch (vos_range) {
  case PWR_REGULATOR_VOLTAGE_SCALE0: {
    if (f_hz <= 42_MHz) {
      return FLASH_LATENCY_0;
    } else if (f_hz <= 84_MHz) {
      return FLASH_LATENCY_1;
    } else if (f_hz <= 126_MHz) {
      return FLASH_LATENCY_2;
    } else if (f_hz <= 168_MHz) {
      return FLASH_LATENCY_3;
    } else if (f_hz <= 210_MHz) {
      return FLASH_LATENCY_4;
    } else if (f_hz <= 250_MHz) {
      return FLASH_LATENCY_5;
    }
    std::unreachable();
  }
  default: std::unreachable();
  }
}

}   // namespace detail

template <ClockSettings CS>
bool ConfigurePowerAndClocks() noexcept {
  using namespace detail;

  static_assert(CS.Validate());

  constexpr auto PllSrcClk = CS.PllSourceClockFrequency();
  constexpr auto SysClkFreq = CS.SysClkSourceClockFrequency();
  constexpr auto HclkFreq = CS.system_clock_settings.AhbClockFrequency(
      SysClkFreq);
  constexpr auto VosRange     = GetVosRange(HclkFreq);
  constexpr auto FlashLatency = GetFlashLatency(HclkFreq, VosRange);

  __HAL_PWR_VOLTAGESCALING_CONFIG(VosRange);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitTypeDef osc_init{
      .OscillatorType      = RCC_OSCILLATORTYPE_HSI,
      .HSIState            = RCC_HSI_ON,
      .HSIDiv              = RCC_HSI_DIV2,
      .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
  };

  // Configure PLL1
  if constexpr (CS.pll.pll1.enable) {
    osc_init.PLL = {
        .PLLState  = RCC_PLL_ON,
        .PLLSource = static_cast<uint32_t>(CS.pll_source),
        .PLLM      = CS.pll.pll1.m,
        .PLLN      = CS.pll.pll1.n,
        .PLLP      = CS.pll.pll1.p,
        .PLLQ      = CS.pll.pll1.q,
        .PLLR      = CS.pll.pll1.r,
        .PLLRGE    = GetShiftedPllVciRange(
            Pll::Pll1, GetPllVciRange(CS.pll.pll1.PllInputFrequency(
                           CS.PllSourceClockFrequency()))),
        .PLLVCOSEL = GetShiftedPllVcoRange(
            Pll::Pll1, GetPllVcoRange(CS.pll.pll1.PllOutputFrequency(
                           CS.PllSourceClockFrequency()))),
        .PLLFRACN = 0,
    };
  } else {
    osc_init.PLL = {.PLLState = RCC_PLL_OFF};
  }

  if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) {
    return false;
  }

  // Configure PLL2 if necessary
  if constexpr (CS.pll.pll2.enable) {
    LL_RCC_PLL2_Disable();
    LL_RCC_PLL2_SetFRACN(0);
    LL_RCC_PLL2_SetM(CS.pll.pll2.m);
    LL_RCC_PLL2_SetN(CS.pll.pll2.n);
    LL_RCC_PLL2_SetP(CS.pll.pll2.p);
    LL_RCC_PLL2_SetQ(CS.pll.pll2.q);
    LL_RCC_PLL2_SetR(CS.pll.pll2.r);
    LL_RCC_PLL2_Enable();

    while (!LL_RCC_PLL2_IsReady()) {
      __asm("nop;");
    }
  }

  // Configure System Clocks
  RCC_ClkInitTypeDef clk_init{
      .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                   | RCC_CLOCKTYPE_PCLK3,
      .SYSCLKSource   = static_cast<uint32_t>(CS.sysclk_source),
      .AHBCLKDivider  = GetAhbDivider(CS.system_clock_settings.ahb_prescaler),
      .APB1CLKDivider = GetApbDivider(CS.system_clock_settings.apb1_prescaler),
      .APB2CLKDivider = GetApbDivider(CS.system_clock_settings.apb2_prescaler),
      .APB3CLKDivider = GetApbDivider(CS.system_clock_settings.apb3_prescaler),
  };

  if (HAL_RCC_ClockConfig(&clk_init, FlashLatency) != HAL_OK) {
    return false;
  }

  return true;
}   // namespace stm32h7

}   // namespace stm32h5