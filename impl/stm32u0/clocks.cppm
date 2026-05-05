module;

#include <chrono>
#include <utility>

#include <stm32u0xx_hal.h>
#include <stm32u0xx_hal_pwr_ex.h>
#include <stm32u0xx_hal_rcc.h>

export module hal.stm32u0:clocks;

import hstd;

namespace stm32u0 {

using namespace hstd::literals;

inline constexpr auto HsiFrequency        = 16_MHz;      //!< HSI clock frequency.
inline constexpr auto Hsi48Frequency      = 48_MHz;      //!< HSI48 clock frequency.
inline constexpr auto LsiFrequency        = 32_kHz;      //!< LSI clock frequency.
inline constexpr auto LseFrequency        = 32'768_Hz;   //!< LSE clock frequency.
inline constexpr auto DefaultHseFrequency = 4_MHz;       //!< Default HSE clock frequency.
inline constexpr auto DefaultMsiFrequency = 4_kHz;       //!< Default MSI clock frequency.

export enum class PllSource : uint32_t {
  Hsi = RCC_PLLSOURCE_HSI,
  Hse = RCC_PLLSOURCE_HSE,
  Msi = RCC_PLLSOURCE_MSI,
};

export struct PllSettings {
  bool enable;   //!< Whether the PLL should be enabled.

  uint32_t m;
  uint32_t n;

  uint32_t p;
  uint32_t q;
  uint32_t r;

  /**
   * @brief Calculates the PLL input frequency (VCI / refx_ck) given the source clock frequency.
   * @param src Source clock frequency.
   * @return refx_ck.
   */
  [[nodiscard]] consteval hstd::Frequency auto
  PllInputFrequency(hstd::Frequency auto src) const noexcept {
    hstd::hertz result = src.template As<hstd::Hz>();
    result /= m;
    return result;
  }

  /**
   * @brief Returns the PLL output frequency (VCO / vcox_ck) given the source clock.
   * @param src Source clock frequency.
   * @return vcox_ck.
   */
  [[nodiscard]] consteval hstd::Frequency auto
  PllOutputFrequency(hstd::Frequency auto src) const noexcept {
    hstd::hertz result = src.template As<hstd::Hz>();
    result /= m;
    result *= n;
    return result;
  }

  /**
   * @brief Calculates the P output clock frequency of the PLL (PLLP).
   * @param src Source clock frequency.
   * @return P output frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto OutputP(hstd::Frequency auto src) const noexcept {
    return Output(src, p);
  }

  /**
   * @brief Calculates the Q output clock frequency of the PLL (PLLQ).
   * @param src Source clock frequency.
   * @return Q output frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto OutputQ(hstd::Frequency auto src) const noexcept {
    return Output(src, q);
  }

  /**
   * @brief Calculates the R output clock frequency of the PLL (PLLR).
   * @param src Source clock frequency.
   * @return R output frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto OutputR(hstd::Frequency auto src) const noexcept {
    return Output(src, r);
  }

 private:
  [[nodiscard]] constexpr hstd::Frequency auto Output(hstd::Frequency auto src,
                                                      uint32_t             div_pqr) const noexcept {
    hstd::hertz result = src.template As<hstd::Hz>();
    result /= m;
    result *= n;
    result /= div_pqr;
    return result;
  }
};

export enum class MsiRange {
  Freq100kHz = RCC_MSIRANGE_0,    //!< MSI at 100 kHz.
  Freq200kHz = RCC_MSIRANGE_1,    //!< MSI at 200 kHz.
  Freq400kHz = RCC_MSIRANGE_2,    //!< MSI at 400 kHz.
  Freq800kHz = RCC_MSIRANGE_3,    //!< MSI at 800 kHz.
  Freq1MHz   = RCC_MSIRANGE_4,    //!< MSI at 1 MHz.
  Freq2MHz   = RCC_MSIRANGE_5,    //!< MSI at 2 MHz.
  Freq4MHz   = RCC_MSIRANGE_6,    //!< MSI at 4 MHz.
  Freq8MHz   = RCC_MSIRANGE_7,    //!< MSI at 8 MHz.
  Freq16MHz  = RCC_MSIRANGE_8,    //!< MSI at 16 MHz.
  Freq24MHz  = RCC_MSIRANGE_9,    //!< MSI at 24 MHz.
  Freq32MHz  = RCC_MSIRANGE_10,   //!< MSI at 32 MHz.
  Freq48MHz  = RCC_MSIRANGE_11,   //!< MSI at 48 MHz.
};

export constexpr hstd::Frequency auto GetMsiFrequency(MsiRange range) noexcept {
  switch (range) {
  case MsiRange::Freq100kHz: return (100_kHz).As<hstd::Hz>();
  case MsiRange::Freq200kHz: return (200_kHz).As<hstd::Hz>();
  case MsiRange::Freq400kHz: return (400_kHz).As<hstd::Hz>();
  case MsiRange::Freq800kHz: return (800_kHz).As<hstd::Hz>();
  case MsiRange::Freq1MHz: return (1_MHz).As<hstd::Hz>();
  case MsiRange::Freq2MHz: return (2_MHz).As<hstd::Hz>();
  case MsiRange::Freq4MHz: return (4_MHz).As<hstd::Hz>();
  case MsiRange::Freq8MHz: return (8_MHz).As<hstd::Hz>();
  case MsiRange::Freq16MHz: return (16_MHz).As<hstd::Hz>();
  case MsiRange::Freq24MHz: return (24_MHz).As<hstd::Hz>();
  case MsiRange::Freq32MHz: return (32_MHz).As<hstd::Hz>();
  case MsiRange::Freq48MHz: return (48_MHz).As<hstd::Hz>();
  default: std::unreachable();
  }
}

inline constexpr MsiRange DefaultMsiRange = MsiRange::Freq4MHz;

export struct SystemClockSettings {
  uint32_t ahb_prescaler;
  uint32_t apb_prescaler;

  /**
   * Validates the System Clock Settings instance. Triggers UB on failure.
   * @param sysclk System source clock frequency.
   * @return \c true.
   */
  [[nodiscard]] consteval bool Validate(hstd::Frequency auto sysclk) const noexcept {
    // Validate prescaler values.
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(ahb_prescaler),
                 "AHB prescaler must have a valid value");
    hstd::Assert(hstd::IsOneOf<1, 2, 4, 8, 16>(apb_prescaler),
                 "APB Prescaler must have a valid value");

    // Validate maximum frequencies
    hstd::Assert(AhbClockFrequency(sysclk).template As<hstd::Hz>() <= (56_MHz).As<hstd::Hz>(),
                 "AHB Frequency (HCLK) may not exceed 56 MHz");

    hstd::Assert(PeripheralsClockFrequency(sysclk).template As<hstd::Hz>()
                     <= (56_MHz).As<hstd::Hz>(),
                 "APB1 peripherals clock (PCLK1) frequency may not exceed 56 MHz");

    return true;
  }

  /**
   * @brief Calculates the AHB clock frequency.
   * @param sysclk System source clock frequency.
   * @return AHB clock frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto
  AhbClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return sysclk / ahb_prescaler;
  }

  /**
   * @brief Calculates the peripherals source clock frequency.
   * @param sysclk System source clock frequency.
   * @return Peripherals source clock frequency (APB frequency).
   */
  [[nodiscard]] consteval hstd::Frequency auto
  PeripheralsClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    return AhbClockFrequency(sysclk) / apb_prescaler;
  }

  /**
   * @brief Calculates the timers source clock frequency.
   * @param sysclk System source clock frequency.
   * @return Timers source clock frequency (APB timers frequency).
   */
  [[nodiscard]] consteval hstd::Frequency auto
  TimersClockFrequency(hstd::Frequency auto sysclk) const noexcept {
    if (apb_prescaler == 1) {
      return Apb1PeripheralsClockFrequency(sysclk);
    } else {
      return Apb1PeripheralsClockFrequency(sysclk) * 2;
    }
  }
};

export inline constexpr SystemClockSettings DefaultSystemClockSettings = {
    .ahb_prescaler = 1,
    .apb_prescaler = 1,
};

export inline constexpr PllSource   DefaultPllSource   = PllSource::Hsi;
export inline constexpr PllSettings DefaultPllSettings = {
    .enable = false,
    .m      = 1,
    .n      = 7,
    .p      = 2,
    .q      = 2,
    .r      = 2,
};

/**
 * @brief Source clock for SYSCLK.
 */
export enum class SysClkSource : uint32_t {
  Lse = RCC_SYSCLKSOURCE_LSE,
  Msi = RCC_SYSCLKSOURCE_MSI,
  Hsi = RCC_SYSCLKSOURCE_HSI,
  Hse = RCC_SYSCLKSOURCE_HSE,
  Pll = RCC_SYSCLKSOURCE_PLLCLK,
  Lsi = RCC_SYSCLKSOURCE_LSI,
};
export inline constexpr auto DefaultSysClkSource = SysClkSource::Hsi;

export enum class VoltageScalingRange {
  HighPerformance = PWR_REGULATOR_VOLTAGE_SCALE1,
  LowPower        = PWR_REGULATOR_VOLTAGE_SCALE2,
};

export inline constexpr auto DefaultVoltageScalingRange = VoltageScalingRange::HighPerformance;

export enum class FlashWaitStates {
  Ws0,   //!< 0WS (1 CPU cycle).
  Ws1,   //!< 1WS (2 CPU cycles).
  Ws2,   //!< 2WS (3 CPU cycles).
};

export struct ClockSettings {
  hstd::hertz         f_hse                 = DefaultHseFrequency.template As<hstd::Hz>();
  MsiRange            msi_range             = DefaultMsiRange;
  PllSource           pll_source            = DefaultPllSource;
  PllSettings         pll_settings          = DefaultPllSettings;
  SysClkSource        sysclk_source         = DefaultSysClkSource;
  SystemClockSettings system_clock_settings = DefaultSystemClockSettings;
  VoltageScalingRange voltage_scaling_range = DefaultVoltageScalingRange;

  /**
   * @brief Returns the HSI (High Speed Internal) clock frequency.
   * @return HSI Frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto HsiFrequency() const noexcept {
    return stm32u0::HsiFrequency.As<hstd::Hz>();
  }

  /**
   * @brief Returns the HSI48 (High Speed Internal 48 MHz / USB) clock
   * frequency.
   * @return HSI Frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto Hsi48Frequency() const noexcept {
    return stm32u0::Hsi48Frequency.As<hstd::Hz>();
  }

  /**
   * @brief Returns the PLL source clock frequency.
   * @return PLL source clock frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto PllSourceClockFrequency() const noexcept {
    switch (pll_source) {
    case PllSource::Hsi: return HsiFrequency().As<hstd::Hz>();
    case PllSource::Msi: return GetMsiFrequency(msi_range).As<hstd::Hz>();
    case PllSource::Hse: return f_hse.As<hstd::Hz>();
    }

    std::unreachable();
  }

  /**
   * @brief Returns the system source clock frequency.
   * @return System source clock frequency.
   */
  [[nodiscard]] consteval hstd::Frequency auto SysClkSourceClockFrequency() const noexcept {
    switch (sysclk_source) {
    case SysClkSource::Lse: return LseFrequency.As<hstd::Hz>();
    case SysClkSource::Msi: return GetMsiFrequency(msi_range).As<hstd::Hz>();
    case SysClkSource::Hsi: return HsiFrequency().As<hstd::Hz>();
    case SysClkSource::Hse: return f_hse.As<hstd::Hz>();
    case SysClkSource::Pll: return pll_settings.OutputP(PllSourceClockFrequency()).As<hstd::Hz>();
    case SysClkSource::Lsi: return LsiFrequency.As<hstd::Hz>();
    }

    std::unreachable();
  }

  /**
   * @grief Returns whether the clock configuration supports the Low-Power VOS range (Range 2). For
   * reference, refer to STM32U0 reference manual (RM0503) section 4.1.6.
   * @return Whether the clock configuration supports the Low-Power VOS range.
   */
  [[nodiscard]] consteval bool SupportsLowPowerVosRange() const noexcept {
    switch (sysclk_source) {
    case SysClkSource::Lse: return true;
    case SysClkSource::Msi: return GetMsiFrequency(msi_range) <= 18_MHz;
    case SysClkSource::Hsi: return true;
    case SysClkSource::Hse: return f_hse <= 18_MHz;
    case SysClkSource::Pll: return pll_settings.OutputP(PllSourceClockFrequency()) <= 18_MHz;
    case SysClkSource::Lsi: return true;
    }
  }

  /**
   * @brief Validates the Clock Settings instance. Triggers UB on failure.
   * @return \c true.
   */
  [[nodiscard]] consteval bool Validate() const noexcept {
    // Validate system source clock
    if (!system_clock_settings.Validate(SysClkSourceClockFrequency())) {
      return false;
    }

    // Validate VOS range.
    if (voltage_scaling_range == VoltageScalingRange::LowPower) {
      hstd::Assert(SupportsLowPowerVosRange(), "Cannot use low-power VOS range (range 2) with "
                                               "configured system source clock frequency.");
    }

    return true;
  }

  /**
   * Returns whether the PLL is enabled (used as a clock source anywhere).
   * @return Whether the PLL is enabled.
   */
  [[nodiscard]] consteval bool IsPllEnabled() const noexcept {
    return sysclk_source == SysClkSource::Pll;
  }

  /**
   * Returns whether the HSI oscillator is enabled (used as a clock source anywhere).
   * @return Whether the HSI oscillator is enabled.
   */
  [[nodiscard]] consteval bool IsHsiEnabled() const noexcept {
    return sysclk_source == SysClkSource::Hsi || (IsPllEnabled() && pll_source == PllSource::Hsi);
  }

  /**
   * Returns whether the MSI oscillator is enabled (used as a clock source anywhere).
   * @return Whether the MSI oscillator is enabled.
   */
  [[nodiscard]] consteval bool IsMsiEnabled() const noexcept {
    return sysclk_source == SysClkSource::Msi || (IsPllEnabled() && pll_source == PllSource::Msi);
  }

  /**
   * Returns whether the LSI oscillator is enabled (used as a clock source anywhere).
   * @return Whether the LSI oscillator is enabled.
   */
  [[nodiscard]] consteval bool IsLsiEnabled() const noexcept {
    return sysclk_source == SysClkSource::Lsi;
  }

  [[nodiscard]] consteval FlashWaitStates GetLowestFlashWaitStates() const noexcept {
    using enum FlashWaitStates;

    const auto hclk = system_clock_settings.AhbClockFrequency(SysClkSourceClockFrequency());
    if (voltage_scaling_range == VoltageScalingRange::HighPerformance) {
      if (hclk <= 24_MHz) {
        return Ws0;
      }
      if (hclk <= 48_MHz) {
        return Ws1;
      }
      return Ws2;
    }

    if (voltage_scaling_range == VoltageScalingRange::LowPower) {
      if (hclk <= 8_MHz) {
        return Ws0;
      }
      if (hclk <= 16_MHz) {
        return Ws1;
      }
      return Ws2;
    }

    return Ws2;
  }
};

export inline constexpr auto DefaultClockSettings = ClockSettings{};

[[nodiscard]] consteval uint32_t GetAhbDivider(uint32_t div) noexcept {
  return hstd::StaticMap<int, uint32_t, 9>(div, {{
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
  return hstd::StaticMap<int, uint32_t, 5>(div, {{
                                                    {1, RCC_HCLK_DIV1},
                                                    std::make_pair(2, RCC_HCLK_DIV2),
                                                    std::make_pair(4, RCC_HCLK_DIV4),
                                                    std::make_pair(8, RCC_HCLK_DIV8),
                                                    std::make_pair(16, RCC_HCLK_DIV16),
                                                }});
}

export template <ClockSettings CS>
bool ConfigurePowerAndClocks() noexcept {
  static_assert(CS.Validate());

  // Configure voltage scaling range.
  HAL_PWREx_ControlVoltageScaling(static_cast<uint32_t>(CS.voltage_scaling_range));

  // Configure oscilator.
  {
    RCC_OscInitTypeDef osc_init{};

    osc_init.OscillatorType =
        RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_LSI;

    if constexpr (CS.IsHsiEnabled()) {
      osc_init.HSIState            = RCC_HSI_ON;
      osc_init.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    }

    if constexpr (CS.IsMsiEnabled()) {
      osc_init.MSIState            = RCC_MSI_ON;
      osc_init.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
      osc_init.MSIClockRange       = static_cast<uint32_t>(CS.msi_range);
    }

    if constexpr (CS.IsLsiEnabled()) {
      osc_init.LSIState = RCC_LSI_ON;
    }

    if constexpr (CS.IsPllEnabled()) {
      osc_init.PLL.PLLState  = RCC_PLL_ON;
      osc_init.PLL.PLLSource = static_cast<uint32_t>(CS.pll_source);
      osc_init.PLL.PLLM      = CS.pll_settings.m;
      osc_init.PLL.PLLN      = CS.pll_settings.n;
      osc_init.PLL.PLLP      = CS.pll_settings.p;
      osc_init.PLL.PLLQ      = CS.pll_settings.q;
      osc_init.PLL.PLLR      = CS.pll_settings.r;
    }

    if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) {
      return false;
    }
  }

  // Configure system clock.
  {
    RCC_ClkInitTypeDef clk_init{
        .ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1,
        .SYSCLKSource   = static_cast<uint32_t>(CS.sysclk_source),
        .AHBCLKDivider  = GetAhbDivider(CS.system_clock_settings.ahb_prescaler),
        .APB1CLKDivider = GetApbDivider(CS.system_clock_settings.apb_prescaler),
    };

    if (HAL_RCC_ClockConfig(&clk_init, static_cast<uint32_t>(CS.GetLowestFlashWaitStates()))
        != HAL_OK) {
      return false;
    }
  }

  return true;
}

/**
 * Clock implementation based on the SysTick interrupt counter.
 */
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


}   // namespace stm32u0