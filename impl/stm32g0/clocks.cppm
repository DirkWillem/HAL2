module;

#include <chrono>
#include <utility>

#include <stm32g0xx_hal.h>

#include "internal/peripheral_availability.h"

export module hal.stm32g0:clocks;

import hstd;

namespace stm32g0 {

using namespace hstd::literals;

inline constexpr auto HsiFrequency        = 16_MHz;
inline constexpr auto Hsi48Frequency      = 48_MHz;
inline constexpr auto LsiFrequency        = 32_kHz;
inline constexpr auto LseFrequency        = 32'768_Hz;
inline constexpr auto DefaultHseFrequency = 8_MHz;

export enum class PllSource : uint32_t {
  Hsi = RCC_PLLSOURCE_HSI,
  Hse = RCC_PLLSOURCE_HSE,
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

export inline constexpr auto        DefaultPllSource   = PllSource::Hsi;
export inline constexpr PllSettings DefaultPllSettings = {
    .enable = true, .m = 1, .n = 8, .p = 4, .q = 2, .r = 2};

export enum class SysClkSource : uint32_t {
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Lsi = RCC_SYSCLKSOURCE_LSI,
  Lse = RCC_SYSCLKSOURCE_LSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK,
};

export struct SystemClockSettings {
  uint32_t ahb_prescaler;
  uint32_t apb_prescaler;
  uint32_t cortex_prescaler;

  [[nodiscard]] consteval bool
  Validate(hstd::Frequency auto sysclk) const noexcept {
    // Validate prescaler values
    hstd::Assert(
        hstd::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(ahb_prescaler),
        "AHB Prescaler must have a valid value");
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16>(apb_prescaler),
                 "APB Prescaler must have a valid value");
    hstd::Assert(hstd::IsOneOf<1, 8>(cortex_prescaler),
                 "Cortex prescaler must have a valid value");

    // Validate maximum frequencies
    hstd::Assert(AhbClockFrequency(sysclk).template As<hstd::Hz>()
                     <= (64_MHz).As<hstd::Hz>(),
                 "AHB Frequency (HCLK) may not exceed 64 MHz");
    hstd::Assert(
        ApbPeripheralsClockFrequency(sysclk).template As<hstd::Hz>()
            <= (64_MHz).As<hstd::Hz>(),
        "APB peripherals clock (PCLK1) frequency may not exceed 64 MHz");

    return true;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  AhbClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return sysclk / ahb_prescaler;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  ApbPeripheralsClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb_prescaler;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  ApbTimersClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    if (apb_prescaler == 1) {
      return ApbPeripheralsClockFrequency(sysclk).template As<hstd::Hz>();
    }
    return (ApbPeripheralsClockFrequency(sysclk) * 2).template As<hstd::Hz>();
  }
};

export inline constexpr auto DefaultSysClkSource = SysClkSource::Hsi;
export inline constexpr SystemClockSettings DefaultSystemClockSettings = {
    .ahb_prescaler = 1, .apb_prescaler = 1, .cortex_prescaler = 1};
export inline constexpr auto DefaultHsiPrescaler = 1;

export struct ClockSettings {
  hstd::hertz         f_hse         = DefaultHseFrequency.As<hstd::Hz>();
  PllSource           pll_source    = DefaultPllSource;
  PllSettings         pll           = DefaultPllSettings;
  uint32_t            hsi_prescaler = DefaultHsiPrescaler;
  SysClkSource        sysclk_source = DefaultSysClkSource;
  SystemClockSettings system_clock_settings = DefaultSystemClockSettings;

  /**
   * Returns the frequency of the HSI clock after applying the HSI prescaler
   * @returns Prescaled HSI frequency
   */
  [[nodiscard]] consteval hstd::Frequency auto
  ScaledHsiFrequency() const noexcept {
    return HsiFrequency / hsi_prescaler;
  }

  [[nodiscard]] consteval hstd::Frequency auto
  PllSourceClockFrequency() const noexcept {
    switch (pll_source) {
    case PllSource::Hsi: return HsiFrequency.As<hstd::Hz>();
    case PllSource::Hse: return f_hse.As<hstd::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval hstd::Frequency auto
  SysClkSourceClockFrequency() const noexcept {
    switch (sysclk_source) {
    case SysClkSource::Hsi: return ScaledHsiFrequency().As<hstd::Hz>();
    case SysClkSource::Hse: return f_hse;
    case SysClkSource::Lsi: return LsiFrequency.As<hstd::Hz>();
    case SysClkSource::Lse: return LseFrequency.As<hstd::Hz>();
    case SysClkSource::Pll:
      return pll.OutputR(PllSourceClockFrequency()).As<hstd::Hz>();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval bool Validate() const noexcept {
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16, 64, 128>(hsi_prescaler),
                 "HSI Prescaler must have a valid value");
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

[[nodiscard]] consteval uint32_t GetHsiPrescaler(uint32_t div) noexcept {
  return hstd::StaticMap<int, uint32_t, 8>(
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
  return ((div - 1) << RCC_PLLCFGR_PLLM_Pos) & RCC_PLLCFGR_PLLM_Msk;
}

[[nodiscard]] consteval uint32_t GetPllPDiv(uint32_t div) noexcept {
  return ((div - 1) << RCC_PLLCFGR_PLLP_Pos) & RCC_PLLCFGR_PLLP_Msk;
}

#ifdef HAS_PLLQ
[[nodiscard]] consteval uint32_t GetPllQDiv(uint32_t div) noexcept {
  return ((div - 1) << RCC_PLLCFGR_PLLQ_Pos) & RCC_PLLCFGR_PLLQ_Msk;
}
#endif

[[nodiscard]] consteval uint32_t GetPllRDiv(uint32_t div) noexcept {
  return ((div - 1) << RCC_PLLCFGR_PLLR_Pos) & RCC_PLLCFGR_PLLR_Msk;
}

[[nodiscard]] consteval uint32_t GetAhbDivider(uint32_t div) noexcept {
  return hstd::StaticMap<int, uint32_t, 9>(
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
  return hstd::StaticMap<int, uint32_t, 5>(
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
GetFlashLatency(hstd::Frequency auto f_hclk, CoreVoltageRange vos_range) {
  const auto f_hz = f_hclk.template As<hstd::Hz>();

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

export template <ClockSettings CS>
bool ConfigurePowerAndClocks() noexcept {
  static_assert(CS.Validate());

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

#ifdef HAS_USBCPD_STROBE_BITS
  HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_CFGR1_UCPD1_STROBE
                                   | SYSCFG_CFGR1_UCPD2_STROBE);
#endif

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
#ifdef HAS_HSI48
      .HSI48State = RCC_HSI48_ON,
#endif
      .PLL =
          {
              .PLLState  = RCC_PLL_OFF,
              .PLLSource = RCC_PLLSOURCE_NONE,
              .PLLM      = RCC_PLLM_DIV1,
              .PLLN      = 8,
              .PLLP      = RCC_PLLP_DIV2,
#ifdef HAS_PLLQ
              .PLLQ = RCC_PLLQ_DIV2,
#endif
              .PLLR = RCC_PLLR_DIV2,
          },
  };

  if (CS.pll.enable) {
    osc_init.PLL = {
        .PLLState  = RCC_PLL_ON,
        .PLLSource = static_cast<uint32_t>(CS.pll_source),
        .PLLM      = GetPllM(CS.pll.m),
        .PLLN      = CS.pll.n,
        .PLLP      = GetPllPDiv(CS.pll.p),
#ifdef HAS_PLLQ
        .PLLQ = GetPllQDiv(CS.pll.q),
#endif
        .PLLR = GetPllRDiv(CS.pll.r),
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

static_assert(hstd::Clock<SysTickClock>);

}   // namespace stm32g0
