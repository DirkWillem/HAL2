module;

#include <chrono>
#include <utility>

#include <stm32h5xx_hal.h>
#include <stm32h5xx_hal_rcc_ex.h>
#include <stm32h5xx_ll_rcc.h>

export module hal.stm32h5:clocks;

import hstd;

namespace stm32h5 {

using namespace hstd::literals;

inline constexpr auto HsiFrequency   = 64_MHz;
inline constexpr auto Hsi48Frequency = 48_MHz;
inline constexpr auto CsiFrequency   = 4_MHz;

export enum class PllSource : uint32_t {
  Hsi = RCC_PLLSOURCE_HSI,
  Csi = RCC_PLLSOURCE_CSI,
  Hse = RCC_PLLSOURCE_HSE
};

export struct PllSettings {
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
  [[nodiscard]] consteval hstd::Frequency auto
  PllInputFrequency(hstd::Frequency auto src) const noexcept {
    hstd::hertz result = src.template As<hstd::Hz>();
    result /= m;
    return result;
  }

  /**
   * Returns the PLL output frequency (VCO / vcox_ck) given the source clock
   * @param src Source clock frequency
   * @return vcox_ck
   */
  [[nodiscard]] consteval hstd::Frequency auto
  PllOutputFrequency(hstd::Frequency auto src) const noexcept {
    hstd::hertz result = src.template As<hstd::Hz>();
    result /= m;
    result *= n;
    return result;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  OutputP(hstd::Frequency auto src) const noexcept {
    return Output(src, p);
  }

  [[nodiscard]] consteval hstd::Frequency auto
  OutputQ(hstd::Frequency auto src) const noexcept {
    return Output(src, q);
  }

  [[nodiscard]] consteval hstd::Frequency auto
  OutputR(hstd::Frequency auto src) const noexcept {
    return Output(src, r);
  }

 private:
  [[nodiscard]] constexpr hstd::Frequency auto
  Output(hstd::Frequency auto src, uint32_t div_pqr) const noexcept {
    hstd::hertz result = src.template As<hstd::Hz>();
    result /= m;
    result *= n;
    result /= div_pqr;
    return result;
  }
};

export struct PllsSettings {
  PllSettings pll1;
  PllSettings pll2;
  PllSettings pll3;
};

export enum class SysClkSource : uint32_t {
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Csi = RCC_SYSCLKSOURCE_CSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK
};

export struct SystemClockSettings {
  uint32_t ahb_prescaler;
  uint32_t apb1_prescaler;
  uint32_t apb2_prescaler;
  uint32_t apb3_prescaler;

  [[nodiscard]] consteval bool
  Validate(hstd::Frequency auto sysclk) const noexcept {
    // Validate D1 prescaler values
    hstd::Assert(
        hstd::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(ahb_prescaler),
        "AHB prescaler must have a valid valeu");

    // Validate all D2 prescaler values
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16>(apb1_prescaler),
                 "APB1 Prescaler must have a valid value");
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16>(apb2_prescaler),
                 "APB2 Prescaler must have a valid value");
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16>(apb3_prescaler),
                 "APB3 Prescaler must have a valid value");

    // Validate maximum frequencies
    hstd::Assert(AhbClockFrequency(sysclk).template As<hstd::Hz>()
                     <= (250_MHz).As<hstd::Hz>(),
                 "AHB Frequency (HCLK) may not exceed 250 MHz");

    hstd::Assert(
        Apb1PeripheralsClockFrequency(sysclk).template As<hstd::Hz>()
            <= (250_MHz).As<hstd::Hz>(),
        "APB1 peripherals clock (PCLK1) frequency may not exceed 250 MHz");
    hstd::Assert(
        Apb2PeripheralsClockFrequency(sysclk).template As<hstd::Hz>()
            <= (250_MHz).As<hstd::Hz>(),
        "APB2 peripherals clock (PCLK2) frequency may not exceed 250 MHz");
    hstd::Assert(
        Apb3PeripheralsClockFrequency(sysclk).template As<hstd::Hz>()
            <= (250_MHz).As<hstd::Hz>(),
        "APB3 peripherals clock (PCLK3) frequency may not exceed 250 MHz");

    return true;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  AhbClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return sysclk / ahb_prescaler;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Apb1PeripheralsClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb1_prescaler;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Apb1TimersClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    if (apb1_prescaler == 1) {
      return Apb1PeripheralsClockFrequency(sysclk);
    } else {
      return Apb1PeripheralsClockFrequency(sysclk) * 2;
    }
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Apb2PeripheralsClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb2_prescaler;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Apb2TimersClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    if (apb2_prescaler == 1) {
      return Apb2PeripheralsClockFrequency(sysclk);
    } else {
      return Apb2PeripheralsClockFrequency(sysclk) * 2;
    }
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Apb3PeripheralsClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb3_prescaler;
  }
};

export inline constexpr PllSource    DefaultPllSource = PllSource::Hsi;
export inline constexpr PllsSettings DefaultPllSettings{
    .pll1 = {.enable = true, .m = 16, .n = 125, .p = 2, .q = 2, .r = 2},
    .pll2 = {},
    .pll3 = {},
};

export inline constexpr SysClkSource DefaultSysClkSource = SysClkSource::Pll;
export inline constexpr SystemClockSettings DefaultSystemClockSettings = {
    .ahb_prescaler  = 1,
    .apb1_prescaler = 1,
    .apb2_prescaler = 1,
    .apb3_prescaler = 1,
};

export struct ClockSettings {
  hstd::hertz         f_hse;
  PllSource           pll1_source           = DefaultPllSource;
  PllSource           pll2_source           = DefaultPllSource;
  PllSource           pll3_source           = DefaultPllSource;
  PllsSettings        pll                   = DefaultPllSettings;
  SysClkSource        sysclk_source         = DefaultSysClkSource;
  SystemClockSettings system_clock_settings = DefaultSystemClockSettings;

  /**
   * @brief Returns the HSI (High Speed Internal) clock frequency.
   * @return HSI Frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto HsiFrequency() const noexcept {
    return stm32h5::HsiFrequency.As<hstd::Hz>();
  }

  /**
   * @brief Returns the HSI48 (High Speed Internal 48 MHz / USB) clock
   * frequency.
   * @return HSI Frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto Hsi48Frequency() const noexcept {
    return stm32h5::Hsi48Frequency.As<hstd::Hz>();
  }

  /**
   * @brief Returns the CSI clock frequency.
   * @return CSI Frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto CsiFrequency() const noexcept {
    return stm32h5::CsiFrequency.As<hstd::Hz>();
  }

  /**
   * @brief Returns the HSE (High Speed External) clock frequency.
   * @return HSE Frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto HseFrequency() const noexcept {
    return f_hse.As<hstd::Hz>();
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Pll1SourceClockFrequency() const noexcept {
    switch (pll1_source) {
    case PllSource::Hsi: return HsiFrequency().As<hstd::Hz>();
    case PllSource::Csi: return CsiFrequency().As<hstd::Hz>();
    case PllSource::Hse: return f_hse.As<hstd::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Pll2SourceClockFrequency() const noexcept {
    switch (pll2_source) {
    case PllSource::Hsi: return HsiFrequency().As<hstd::Hz>();
    case PllSource::Csi: return CsiFrequency().As<hstd::Hz>();
    case PllSource::Hse: return f_hse.As<hstd::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval hstd::Frequency auto
  Pll3SourceClockFrequency() const noexcept {
    switch (pll3_source) {
    case PllSource::Hsi: return HsiFrequency().As<hstd::Hz>();
    case PllSource::Csi: return CsiFrequency().As<hstd::Hz>();
    case PllSource::Hse: return f_hse.As<hstd::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval hstd::Frequency auto
  SysClkSourceClockFrequency() const noexcept {
    switch (sysclk_source) {
    case SysClkSource::Hsi: return HsiFrequency().As<hstd::Hz>();
    case SysClkSource::Csi: return CsiFrequency().As<hstd::Hz>();
    case SysClkSource::Hse: return f_hse.As<hstd::Hz>();
    case SysClkSource::Pll:
      return pll.pll1.OutputP(Pll1SourceClockFrequency()).As<hstd::Hz>();
    }

    std::unreachable();
  }

  /**
   * @brief Returns the HCLK (AHB) frequency.
   * @return HCLK frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto HclkFrequency() const noexcept {
    return (SysClkSourceClockFrequency() / system_clock_settings.ahb_prescaler)
        .As<hstd::Hz>();
  }

  [[nodiscard]] consteval bool Validate() const noexcept {
    return system_clock_settings.Validate(
        SysClkSourceClockFrequency().As<hstd::Hz>());
  }
};

export inline constexpr ClockSettings DefaultClockSettings{
    .f_hse                 = (25_MHz).As<hstd::Hz>(),
    .pll1_source           = PllSource::Hsi,
    .pll                   = DefaultPllSettings,
    .sysclk_source         = SysClkSource::Pll,
    .system_clock_settings = DefaultSystemClockSettings,
};

static_assert(DefaultClockSettings.Validate());

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
GetPllVciRange(hstd::Frequency auto vci_freq) noexcept {
  const auto f_hz = vci_freq.template As<hstd::Hz>();

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
GetPllVcoRange(hstd::Frequency auto vco_freq) noexcept {
  const auto f_hz = vco_freq.template As<hstd::Hz>();

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
  return hstd::StaticMap<int, uint32_t, 9>(
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
  return hstd::StaticMap<int, uint32_t, 5>(
      div, {{
               {1, RCC_HCLK_DIV1},
               std::make_pair(2, RCC_HCLK_DIV2),
               std::make_pair(4, RCC_HCLK_DIV4),
               std::make_pair(8, RCC_HCLK_DIV8),
               std::make_pair(16, RCC_HCLK_DIV16),
           }});
}

[[nodiscard]] consteval uint32_t
GetVosRange([[maybe_unused]] hstd::Frequency auto f_hclk) noexcept {
  // TODO: Properly handle this?
  return PWR_REGULATOR_VOLTAGE_SCALE0;
}

[[nodiscard]] consteval uint32_t GetFlashLatency(hstd::Frequency auto f_hclk,
                                                 uint32_t vos_range) {
  const auto f_hz = f_hclk.template As<hstd::Hz>();

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

export template <ClockSettings CS>
bool ConfigurePowerAndClocks() noexcept {
  static_assert(CS.Validate());

  constexpr auto PllSrcClk  = CS.Pll1SourceClockFrequency();
  constexpr auto SysClkFreq = CS.SysClkSourceClockFrequency();
  constexpr auto HclkFreq =
      CS.system_clock_settings.AhbClockFrequency(SysClkFreq);
  constexpr auto VosRange     = GetVosRange(HclkFreq);
  constexpr auto FlashLatency = GetFlashLatency(HclkFreq, VosRange);

  __HAL_PWR_VOLTAGESCALING_CONFIG(VosRange);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitTypeDef osc_init{
      .OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48,
      .HSEState            = RCC_HSE_OFF,
      .LSEState            = RCC_LSE_OFF,
      .HSIState            = RCC_HSI_ON,
      .HSIDiv              = RCC_HSI_DIV1,
      .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
      .LSIState            = RCC_LSI_OFF,
      .CSIState            = RCC_CSI_OFF,
      .CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT,
      .HSI48State          = RCC_HSI48_ON,
      .PLL                 = {},
  };

  // Configure PLL1
  if constexpr (CS.pll.pll1.enable) {
    osc_init.PLL = {
        .PLLState  = RCC_PLL_ON,
        .PLLSource = static_cast<uint32_t>(CS.pll1_source),
        .PLLM      = CS.pll.pll1.m,
        .PLLN      = CS.pll.pll1.n,
        .PLLP      = CS.pll.pll1.p,
        .PLLQ      = CS.pll.pll1.q,
        .PLLR      = CS.pll.pll1.r,
        .PLLRGE    = GetShiftedPllVciRange(
            Pll::Pll1, GetPllVciRange(CS.pll.pll1.PllInputFrequency(
                           CS.Pll1SourceClockFrequency()))),
        .PLLVCOSEL = GetShiftedPllVcoRange(
            Pll::Pll1, GetPllVcoRange(CS.pll.pll1.PllOutputFrequency(
                           CS.Pll1SourceClockFrequency()))),
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
    RCC_PLL2InitTypeDef pll2_init{
        .PLL2Source = static_cast<uint32_t>(CS.pll2_source),
        .PLL2M      = CS.pll.pll2.m,
        .PLL2N      = CS.pll.pll2.n,
        .PLL2P      = CS.pll.pll2.p,
        .PLL2Q      = CS.pll.pll2.q,
        .PLL2R      = CS.pll.pll2.r,
        .PLL2RGE    = GetShiftedPllVciRange(
            Pll::Pll2, GetPllVciRange(CS.pll.pll2.PllInputFrequency(
                           CS.Pll2SourceClockFrequency()))),
        .PLL2VCOSEL = GetShiftedPllVcoRange(
            Pll::Pll2, GetPllVcoRange(CS.pll.pll2.PllInputFrequency(
                           CS.Pll2SourceClockFrequency()))),
        .PLL2FRACN = 0,
    };
    if (HAL_RCCEx_EnablePLL2(&pll2_init) != HAL_OK) {
      return false;
    }
  }

  // Configure PLL3 if necessary
  if constexpr (CS.pll.pll3.enable) {
    RCC_PLL3InitTypeDef pll3_init{
        .PLL3Source = static_cast<uint32_t>(CS.pll2_source),
        .PLL3M      = CS.pll.pll3.m,
        .PLL3N      = CS.pll.pll3.n,
        .PLL3P      = CS.pll.pll3.p,
        .PLL3Q      = CS.pll.pll3.q,
        .PLL3R      = CS.pll.pll3.r,
        .PLL3RGE    = GetShiftedPllVciRange(
            Pll::Pll3, GetPllVciRange(CS.pll.pll3.PllInputFrequency(
                           CS.Pll3SourceClockFrequency()))),
        .PLL3VCOSEL = GetShiftedPllVcoRange(
            Pll::Pll3, GetPllVcoRange(CS.pll.pll3.PllInputFrequency(
                           CS.Pll3SourceClockFrequency()))),
        .PLL3FRACN = 0,
    };
    if (HAL_RCCEx_EnablePLL3(&pll3_init) != HAL_OK) {
      return false;
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
}

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

}   // namespace stm32h5