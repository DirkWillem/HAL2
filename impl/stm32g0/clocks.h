#pragma once

#include <cassert>

#include <constexpr_tools/chrono_ex.h>
#include <constexpr_tools/static_mapping.h>

#include <halstd/logic.h>

#include <stm32g0xx_hal.h>

namespace stm32g0 {

using namespace ct::literals;

inline constexpr auto HsiFrequency        = 16_MHz;
inline constexpr auto Hsi48Frequency      = 48_MHz;
inline constexpr auto LsiFrequency        = 32_kHz;
inline constexpr auto LseFrequency        = 32'768_Hz;
inline constexpr auto DefaultHseFrequency = 8_MHz;

enum class PllSource : uint32_t {
  Hsi = RCC_PLLSOURCE_HSI,
  Hse = RCC_PLLSOURCE_HSE,
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

inline constexpr auto        DefaultPllSource   = PllSource::Hsi;
inline constexpr PllSettings DefaultPllSettings = {
    .enable = true, .m = 1, .n = 8, .p = 4, .q = 2, .r = 2};

enum class SysClkSource : uint32_t {
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Lsi = RCC_SYSCLKSOURCE_LSI,
  Lse = RCC_SYSCLKSOURCE_LSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK,
};

struct SystemClockSettings {
  uint32_t ahb_prescaler;
  uint32_t apb_prescaler;
  uint32_t cortex_prescaler;

  [[nodiscard]] consteval bool
  Validate(ct::Frequency auto sysclk) const noexcept {
    // Validate prescaler values
    assert(("AHB Prescaler must have a valid value",
            halstd::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(ahb_prescaler)));
    assert(("APB Prescaler must have a valid value",
            halstd::IsOneOf<1, 2, 4, 8, 16>(apb_prescaler)));
    assert(("Cortex prescaler must have a valid value",
            halstd::IsOneOf<1, 8>(cortex_prescaler)));

    // Validate maximum frequencies
    assert(("AHB Frequency (HCLK) may not exceed 64 MHz",
            AhbClockFrequency(sysclk).template As<ct::Hz>()
                <= (64_MHz).As<ct::Hz>()));
    assert(("APB peripherals clock (PCLK1) frequency may not exceed 64 MHz",
            ApbPeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (64_MHz).As<ct::Hz>()));

    return true;
  }

  [[nodiscard]] consteval ct::Frequency auto
  AhbClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return sysclk / ahb_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  ApbPeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  ApbTimersClockFrequency(ct::Frequency auto sysclk) const noexcept {
    if (apb_prescaler == 1) {
      return ApbPeripheralsClockFrequency(sysclk).template As<ct::Hz>();
    } else {
      return (ApbPeripheralsClockFrequency(sysclk) * 2).template As<ct::Hz>();
    }
  }
};

inline constexpr auto                DefaultSysClkSource = SysClkSource::Hsi;
inline constexpr SystemClockSettings DefaultSystemClockSettings = {
    .ahb_prescaler = 1, .apb_prescaler = 1, .cortex_prescaler = 1};
inline constexpr auto DefaultHsiPrescaler = 1;

struct ClockSettings {
  ct::hertz           f_hse                 = DefaultHseFrequency.As<ct::Hz>();
  PllSource           pll_source            = DefaultPllSource;
  PllSettings         pll                   = DefaultPllSettings;
  uint32_t            hsi_prescaler         = DefaultHsiPrescaler;
  SysClkSource        sysclk_source         = DefaultSysClkSource;
  SystemClockSettings system_clock_settings = DefaultSystemClockSettings;

  /**
   * Returns the frequency of the HSI clock after applying the HSI prescaler
   * @returns Prescaled HSI frequency
   */
  [[nodiscard]] consteval ct::Frequency auto
  ScaledHsiFrequency() const noexcept {
    return HsiFrequency / hsi_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  PllSourceClockFrequency() const noexcept {
    switch (pll_source) {
    case PllSource::Hsi: return HsiFrequency.As<ct::Hz>();
    case PllSource::Hse: return f_hse.As<ct::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval ct::Frequency auto
  SysClkSourceClockFrequency() const noexcept {
    switch (sysclk_source) {
    case SysClkSource::Hsi: return ScaledHsiFrequency().As<ct::Hz>();
    case SysClkSource::Hse: return f_hse;
    case SysClkSource::Lsi: return LsiFrequency.As<ct::Hz>();
    case SysClkSource::Lse: return LseFrequency.As<ct::Hz>();
    case SysClkSource::Pll:
      return pll.OutputR(PllSourceClockFrequency()).As<ct::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval bool Validate() const noexcept {
    assert(("HSI Prescaler must have a valid value",
            halstd::IsOneOf<1, 2, 4, 8, 16, 64, 128>(hsi_prescaler)));
    return system_clock_settings.Validate(SysClkSourceClockFrequency());
  }
};

enum class CoreVoltageRange {
  Range1 = PWR_REGULATOR_VOLTAGE_SCALE1,
  Range2 = PWR_REGULATOR_VOLTAGE_SCALE2
};

enum class FlashLatency {
  Ws0 = FLASH_LATENCY_0,
  Ws1 = FLASH_LATENCY_1,
  Ws2 = FLASH_LATENCY_2,
};

namespace detail {

[[nodiscard]] consteval uint32_t GetHsiPrescaler(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 8>(
      div, {{
               std::make_pair(1, RCC_HSI_DIV1),
               std::make_pair(2, RCC_HSI_DIV2),
               std::make_pair(4, RCC_HSI_DIV4),
               std::make_pair(8, RCC_HSI_DIV8),
               std::make_pair(16, RCC_HSI_DIV16),
               std::make_pair(32, RCC_HSI_DIV32),
               std::make_pair(64, RCC_HSI_DIV64),
               std::make_pair(128, RCC_HSI_DIV128),
           }});
}

[[nodiscard]] consteval uint32_t GetPllM(uint32_t div) noexcept {
  return (div - 1 << RCC_PLLCFGR_PLLM_Pos) & RCC_PLLCFGR_PLLM_Msk;
}

[[nodiscard]] consteval uint32_t GetPllPDiv(uint32_t div) noexcept {
  return ((div - 1) << RCC_PLLCFGR_PLLP_Pos) & RCC_PLLCFGR_PLLP_Msk;
}

[[nodiscard]] consteval uint32_t GetPllQDiv(uint32_t div) noexcept {
  return ((div - 1) << RCC_PLLCFGR_PLLQ_Pos) & RCC_PLLCFGR_PLLQ_Msk;
}

[[nodiscard]] consteval uint32_t GetPllRDiv(uint32_t div) noexcept {
  return ((div - 1) << RCC_PLLCFGR_PLLR_Pos) & RCC_PLLCFGR_PLLR_Msk;
}

[[nodiscard]] consteval uint32_t GetAhbDivider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 9>(
      div, {
               {
                   std::make_pair(1, RCC_SYSCLK_DIV1),
                   std::make_pair(2, RCC_SYSCLK_DIV2),
                   std::make_pair(4, RCC_SYSCLK_DIV4),
                   std::make_pair(8, RCC_SYSCLK_DIV8),
                   std::make_pair(16, RCC_SYSCLK_DIV16),
                   std::make_pair(64, RCC_SYSCLK_DIV64),
                   std::make_pair(128, RCC_SYSCLK_DIV128),
                   std::make_pair(256, RCC_SYSCLK_DIV256),
                   std::make_pair(512, RCC_SYSCLK_DIV512),
               },
           });
}
[[nodiscard]] consteval uint32_t GetApbDivider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 5>(
      div, {
               {
                   {1, RCC_HCLK_DIV1},
                   std::make_pair(2, RCC_HCLK_DIV2),
                   std::make_pair(4, RCC_HCLK_DIV4),
                   std::make_pair(8, RCC_HCLK_DIV8),
                   std::make_pair(16, RCC_HCLK_DIV16),
               },
           });
}

[[nodiscard]] consteval FlashLatency
GetFlashLatency(ct::Frequency auto f_hclk, CoreVoltageRange vos_range) {
  const auto f_hz = f_hclk.template As<ct::Hz>();

  switch (vos_range) {
  case CoreVoltageRange::Range1:
    if (f_hz <= 24_MHz) {
      return FlashLatency::Ws0;
    } else if (f_hz <= 48_MHz) {
      return FlashLatency::Ws1;
    } else if (f_hz <= 64_MHz) {
      return FlashLatency::Ws2;
    }

    std::unreachable();
  case CoreVoltageRange::Range2:
    if (f_hz <= 8_MHz) {
      return FlashLatency::Ws0;
    } else if (f_hz <= 16_MHz) {
      return FlashLatency::Ws1;
    }

    std::unreachable();
  default: std::unreachable();
  }
}

}   // namespace detail

template <ClockSettings CS>
bool ConfigurePowerAndClocks() noexcept {
  using namespace detail;

  static_assert(CS.Validate());

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_CFGR1_UCPD1_STROBE
                                   | SYSCFG_CFGR1_UCPD2_STROBE);

  constexpr auto vos_range = CoreVoltageRange::Range1;
  HAL_PWREx_ControlVoltageScaling(static_cast<uint32_t>(vos_range));

  // Initialize oscillators
  RCC_OscInitTypeDef osc_init{
      .OscillatorType      = RCC_OSCILLATORTYPE_HSI,
      .HSEState            = RCC_HSE_OFF,
      .LSEState            = RCC_LSE_OFF,
      .HSIState            = RCC_HSI_ON,
      .HSIDiv              = GetHsiPrescaler(CS.hsi_prescaler),
      .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
      .LSIState            = RCC_LSI_OFF,
      .HSI48State          = RCC_HSI48_ON,
      .PLL                 = {.PLLState = RCC_PLL_OFF},
  };

  if (CS.pll.enable) {
    osc_init.PLL = {
        .PLLState  = RCC_PLL_ON,
        .PLLSource = static_cast<uint32_t>(CS.pll_source),
        .PLLM      = GetPllM(CS.pll.m),
        .PLLN      = CS.pll.n,
        .PLLP      = GetPllPDiv(CS.pll.p),
        .PLLQ      = GetPllQDiv(CS.pll.q),
        .PLLR      = GetPllRDiv(CS.pll.r),
    };
  }

  // Somehow this is required due to the PLL not being de-initialized?
  HAL_RCC_DeInit();
  if (const auto result = HAL_RCC_OscConfig(&osc_init); result != HAL_OK) {
    return false;
  }

  // Initialize Core, AHB and APB clocks
  constexpr auto SysClkFreq = CS.SysClkSourceClockFrequency();
  constexpr auto HclkFreq =
      CS.system_clock_settings.AhbClockFrequency(SysClkFreq);
  constexpr auto FlashLatency = GetFlashLatency(HclkFreq, vos_range);

  RCC_ClkInitTypeDef clk_init{
      .ClockType =
          RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_SYSCLK,
      .SYSCLKSource   = static_cast<uint32_t>(CS.sysclk_source),
      .AHBCLKDivider  = GetAhbDivider(CS.system_clock_settings.ahb_prescaler),
      .APB1CLKDivider = GetApbDivider(CS.system_clock_settings.apb_prescaler),
  };

  if (const auto result =
          HAL_RCC_ClockConfig(&clk_init, static_cast<uint32_t>(FlashLatency));
      result != HAL_OK) {
    return false;
  }

  return true;
}

}   // namespace stm32g0
