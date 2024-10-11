#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <stm32h7xx_hal.h>
#include <stm32h7xx_ll_rcc.h>

#include <hal/clocks.h>

#include <constexpr_tools/chrono_ex.h>
#include <constexpr_tools/helpers.h>
#include <constexpr_tools/logic.h>
#include <constexpr_tools/math.h>
#include <constexpr_tools/static_mapping.h>

#include <stm32h7/peripheral_ids.h>

namespace stm32h7 {

using namespace ct::literals;

inline constexpr auto HsiFrequency  = 64_MHz;
inline constexpr auto Rc48Frequency = 48_MHz;
inline constexpr auto CsiFrequency  = 4_MHz;

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
    result *= n;
    result /= m;
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
    result *= n;
    result /= m;
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
  /** D1 domain Core prescaler (D1CPRE) */
  uint32_t d1_core_prescaler;
  /** D1 domain AHB prescaler (HPRE) */
  uint32_t d1_ahb_prescaler;
  /** D1 domain APB3 prescaler (D1PPRE) */
  uint32_t d1_apb3_prescaler;

  /** D2 domain APB1 prescaler (D2PPRE1) */
  uint32_t d2_apb1_prescaler;
  /** D2 domain APB2 prescaler (D2PPRE2) */
  uint32_t d2_apb2_prescaler;

  /** D3 domain APB4 prescaler (D3PPRE) */
  uint32_t d3_apb4_prescaler;

  uint32_t cpu1_systick_prescaler = 1;
  uint32_t cpu2_systick_prescaler = 1;

  [[nodiscard]] consteval bool
  Validate(ct::Frequency auto sysclk) const noexcept {
    // Validate D1 prescaler values
    assert(("D1CPRE must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(d1_core_prescaler)));
    assert(("HPRE must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16, 64, 128, 256, 512>(d1_ahb_prescaler)));
    assert(("D1PPRE must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(d1_apb3_prescaler)));

    // Validate all D2 prescaler values
    assert(("D2PPRE1 must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(d2_apb1_prescaler)));
    assert(("D2PPRE2 must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(d2_apb2_prescaler)));

    // Validate all D3 domain prescaler values
    assert(("D3PPRE must have a valid value",
            ct::IsOneOf<1, 2, 4, 8, 16>(d3_apb4_prescaler)));

    // Validate SysTick prescalers
    assert(("CPU1 SysTick prescaler must be either 1 or 8",
            (cpu1_systick_prescaler == 1 || cpu1_systick_prescaler == 8)));
    assert(("CPU2 SysTick prescaler must be either 1 or 8",
            (cpu2_systick_prescaler == 1 || cpu2_systick_prescaler == 8)));

    // Validate maximum frequencies
    assert(("D1 Core Frequency may not exceed 480 MHz",
            D1CoreFrequency(sysclk).template As<ct::Hz>()
                <= (480_MHz).As<ct::Hz>()));
    assert(("D1 AHB Frequency may not exceed 240 MHz",
            D1AhbFrequency(sysclk).template As<ct::Hz>()
                <= (240_MHz).As<ct::Hz>()));

    assert(("APB1 peripherals clock frequency may not exceed 120 MHz",
            Apb1PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (120_MHz).As<ct::Hz>()));
    assert(("APB2 peripherals clock frequency may not exceed 120 MHz",
            Apb2PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (120_MHz).As<ct::Hz>()));
    assert(("APB3 peripherals clock frequency may not exceed 120 MHz",
            Apb3PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (120_MHz).As<ct::Hz>()));
    assert(("APB4 peripherals clock frequency may not exceed 120 MHz",
            Apb4PeripheralsClockFrequency(sysclk).template As<ct::Hz>()
                <= (120_MHz).As<ct::Hz>()));

    return true;
  }

  [[nodiscard]] consteval ct::Frequency auto
  D1CoreFrequency(ct::Frequency auto sysclk) const noexcept {
    return sysclk / d1_core_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  D1AhbFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1CoreFrequency(sysclk) / d1_ahb_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Cpu1ClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1CoreFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Cpu1SystickClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1CoreFrequency(sysclk) / cpu1_systick_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Cpu2ClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Cpu2SystickClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk) / cpu2_systick_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  AxiPeripheralClosk(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Ahb1PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb1PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk) / d2_apb1_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb1TimersClockFrequency(ct::Frequency auto sysclk) const noexcept {
    if (d2_apb1_prescaler == 1) {
      return Apb1PeripheralsClockFrequency(sysclk);
    } else {
      return Apb1PeripheralsClockFrequency(sysclk) * 2;
    }
  }

  [[nodiscard]] consteval ct::Frequency auto
  Ahb2PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return Ahb1ClockFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb2PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk) / d2_apb2_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb2TimersClockFrequency(ct::Frequency auto sysclk) const noexcept {
    if (d2_apb2_prescaler == 1) {
      return Apb2PeripheralsClockFrequency(sysclk);
    } else {
      return Apb2PeripheralsClockFrequency(sysclk) * 2;
    }
  }

  [[nodiscard]] consteval ct::Frequency auto
  Ahb3PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb3PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk) / d1_apb3_prescaler;
  }

  [[nodiscard]] consteval ct::Frequency auto
  Ahb4PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk);
  }

  [[nodiscard]] consteval ct::Frequency auto
  Apb4PeripheralsClockFrequency(ct::Frequency auto sysclk) const noexcept {
    return D1AhbFrequency(sysclk) / d3_apb4_prescaler;
  }
};

inline constexpr PllSource    DefaultPllSource = PllSource::Hsi;
inline constexpr PllsSettings DefaultPllSettings{
    .pll1 =
        {
            .enable = true,
            .m      = 5,
            .n      = 48,
            .p      = 2,
            .q      = 5,
            .r      = 2,
        },
    .pll2 = {.enable = false, .m = 32, .n = 129, .p = 2, .q = 2, .r = 2},
    .pll3 = {.enable = false, .m = 32, .n = 129, .p = 2, .q = 2, .r = 3}};

inline constexpr SysClkSource        DefaultSysClkSource = SysClkSource::Pll;
inline constexpr SystemClockSettings DefaultSystemClockSettings{
    .d1_core_prescaler      = 1,
    .d1_ahb_prescaler       = 1,
    .d1_apb3_prescaler      = 1,
    .d2_apb1_prescaler      = 1,
    .d2_apb2_prescaler      = 1,
    .d3_apb4_prescaler      = 1,
    .cpu1_systick_prescaler = 1,
    .cpu2_systick_prescaler = 1,
};

enum class PerClkSource : uint32_t {
  Hsi = RCC_CLKPSOURCE_HSI,
  Csi = RCC_CLKPSOURCE_CSI,
  Hse = RCC_CLKPSOURCE_HSE,
};

enum class Spi123ClkSource : uint32_t {
  Pll1Q   = LL_RCC_SPI123_CLKSOURCE_PLL1Q,
  Pll2P   = LL_RCC_SPI123_CLKSOURCE_PLL2P,
  Pll3P   = LL_RCC_SPI123_CLKSOURCE_PLL3P,
  I2sCkIn = LL_RCC_SPI123_CLKSOURCE_I2S_CKIN,
  PerCk   = LL_RCC_SPI123_CLKSOURCE_CLKP,
};

enum class Spi45ClkSource : uint32_t {
  Pclk2 = LL_RCC_SPI45_CLKSOURCE_PCLK2,
  Pll2Q = LL_RCC_SPI45_CLKSOURCE_PLL2Q,
  Pll3R = LL_RCC_SPI45_CLKSOURCE_PLL3Q,
  Hsi   = LL_RCC_SPI45_CLKSOURCE_HSI,
  Csi   = LL_RCC_SPI45_CLKSOURCE_CSI,
  Hse   = LL_RCC_SPI45_CLKSOURCE_HSE,
};

enum class Spi6ClkSource : uint32_t {
  Pclk4 = LL_RCC_SPI6_CLKSOURCE_PCLK4,
  Pll2Q = LL_RCC_SPI6_CLKSOURCE_PLL2Q,
  Pll3Q = LL_RCC_SPI6_CLKSOURCE_PLL3Q,
  Hsi   = LL_RCC_SPI6_CLKSOURCE_HSI,
  Csi   = LL_RCC_SPI6_CLKSOURCE_CSI,
  Hse   = LL_RCC_SPI6_CLKSOURCE_HSE,
};

inline constexpr auto DefaultPerClkSource    = PerClkSource::Hsi;
inline constexpr auto DefaultSpi123ClkSource = Spi123ClkSource::Pll1Q;
inline constexpr auto DefaultSpi45ClkSource  = Spi45ClkSource::Pclk2;
inline constexpr auto DefaultSpi6ClkSource   = Spi6ClkSource::Pclk4;

struct PeripheralSourceClocks {
  Spi123ClkSource spi123 = DefaultSpi123ClkSource;
  Spi45ClkSource  spi45  = DefaultSpi45ClkSource;
  Spi6ClkSource   spi6   = DefaultSpi6ClkSource;
};

struct ClockSettings {
  ct::hertz           f_hse;
  PllSource           pll_source = DefaultPllSource;
  PllsSettings        pll;
  SysClkSource        sysclk_source = DefaultSysClkSource;
  SystemClockSettings system_clock_settings;

  PerClkSource           per_clk_source = DefaultPerClkSource;
  PeripheralSourceClocks peripherals    = {};

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

  [[nodiscard]] consteval ct::Frequency auto
  PerClockFrequency() const noexcept {
    switch (per_clk_source) {
    case PerClkSource::Hsi: return HsiFrequency.As<ct::Hz>();
    case PerClkSource::Csi: return CsiFrequency.As<ct::Hz>();
    case PerClkSource::Hse: return f_hse;
    }

    std::unreachable();
  }

  /**
   * Returns the source clock frequency for the given SPI instance
   * @param spi SPI instance
   * @return Source clock frequency for the given SPI instance
   */
  [[nodiscard]] consteval ct::Frequency auto
  PeripheralSourceClockFrequency(SpiId spi) const noexcept {
    const ct::Frequency auto pll_src    = PllSourceClockFrequency();
    const ct::Frequency auto sysclk_src = SysClkSourceClockFrequency();

    switch (spi) {
    case SpiId::Spi1: [[fallthrough]];
    case SpiId::Spi2: [[fallthrough]];
    case SpiId::Spi3:
      switch (peripherals.spi123) {
      case Spi123ClkSource::Pll1Q: return pll.pll1.OutputQ(pll_src);
      case Spi123ClkSource::Pll2P:
        assert(("PLL2 must be enabled to use PLL2P as source clock",
                pll.pll2.enable));
        return pll.pll2.OutputP(pll_src);
      case Spi123ClkSource::Pll3P:
        assert(("PLL3 must be enabled to use PLL3P as source clock",
                pll.pll3.enable));
        return pll.pll3.OutputP(pll_src);
      case Spi123ClkSource::I2sCkIn: assert(("Not implemented yet", false));
      case Spi123ClkSource::PerCk: return PerClockFrequency();
      }

      std::unreachable();
    case SpiId::Spi4: [[fallthrough]];
    case SpiId::Spi5:
      switch (peripherals.spi45) {
      case Spi45ClkSource::Pclk2:
        return system_clock_settings.Apb2PeripheralsClockFrequency(sysclk_src);
      case Spi45ClkSource::Pll2Q:
        assert(("PLL2 must be enabled to use PLL2Q as source clock",
                pll.pll2.enable));
        return pll.pll2.OutputQ(pll_src);
      case Spi45ClkSource::Pll3R:
        break;
        assert(("PLL3 must be enabled to use PLL3Q as source clock",
                pll.pll3.enable));
        return pll.pll3.OutputQ(pll_src);
      case Spi45ClkSource::Hsi: return HsiFrequency.As<ct::Hz>();
      case Spi45ClkSource::Csi: return CsiFrequency.As<ct::Hz>();
      case Spi45ClkSource::Hse: return f_hse;
      }

      std::unreachable();
    case SpiId::Spi6:
      switch (peripherals.spi6) {
      case Spi6ClkSource::Pclk4:
        return system_clock_settings.Apb4PeripheralsClockFrequency(sysclk_src);
      case Spi6ClkSource::Pll2Q:
        assert(("PLL2 must be enabled to use PLL2Q as source clock",
                pll.pll2.enable));
        return pll.pll2.OutputQ(pll_src);
      case Spi6ClkSource::Pll3Q:
        assert(("PLL3 must be enabled to use PLL3RQas source clock",
                pll.pll3.enable));
        return pll.pll3.OutputQ(pll_src);
      case Spi6ClkSource::Hsi: return HsiFrequency.As<ct::Hz>();
      case Spi6ClkSource::Csi: return CsiFrequency.As<ct::Hz>();
      case Spi6ClkSource::Hse: return f_hse;
      }

      std::unreachable();
    }

    std::unreachable();
  }

  [[nodiscard]] consteval bool Validate() const noexcept {
    // Unimplemented features
    assert(("PLL3 configuration is not yet implemented", !pll.pll3.enable));

    return system_clock_settings.Validate(
        SysClkSourceClockFrequency().As<ct::Hz>());
  }
};

inline constexpr ClockSettings DefaultClockSettings{
    .f_hse                 = (25_MHz).As<ct::Hz>(),
    .pll_source            = PllSource::Hsi,
    .pll                   = DefaultPllSettings,
    .sysclk_source         = SysClkSource::Hsi,
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
    return static_cast<uint32_t>(range) << RCC_PLLCFGR_PLL1RGE_Pos;
  case Pll::Pll2:
    return static_cast<uint32_t>(range) << RCC_PLLCFGR_PLL2RGE_Pos;
  case Pll::Pll3:
    return static_cast<uint32_t>(range) << RCC_PLLCFGR_PLL3RGE_Pos;
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
              == RCC_PLL1VCIRANGE_3);

enum class PllVcoRange {
  Wide   = LL_RCC_PLLVCORANGE_WIDE,
  Medium = LL_RCC_PLLVCORANGE_MEDIUM,
};

[[nodiscard]] consteval PllVcoRange
GetPllVcoRange(ct::Frequency auto vco_freq) noexcept {
  const auto f_hz = vco_freq.template As<ct::Hz>();

  if (f_hz >= 192_MHz && f_hz <= 960_MHz) {
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
    return static_cast<uint32_t>(range) << RCC_PLLCFGR_PLL1VCOSEL_Pos;
  case Pll::Pll2:
    return static_cast<uint32_t>(range) << RCC_PLLCFGR_PLL2VCOSEL_Pos;
  case Pll::Pll3:
    return static_cast<uint32_t>(range) << RCC_PLLCFGR_PLL3VCOSEL_Pos;
  }

  std::unreachable();
}

[[nodiscard]] consteval uint32_t GetSysClkDivider(uint32_t div) noexcept {
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

[[nodiscard]] consteval uint32_t GetAhbDivider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 9>(
      div, {{
               std::make_pair(1, RCC_HCLK_DIV1),
               std::make_pair(2, RCC_HCLK_DIV2),
               std::make_pair(4, RCC_HCLK_DIV4),
               std::make_pair(8, RCC_HCLK_DIV8),
               std::make_pair(16, RCC_HCLK_DIV16),
               std::make_pair(64, RCC_HCLK_DIV64),
               std::make_pair(128, RCC_HCLK_DIV128),
               std::make_pair(256, RCC_HCLK_DIV256),
               std::make_pair(512, RCC_HCLK_DIV512),
           }});
}

[[nodiscard]] consteval uint32_t GetApb1Divider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 5>(div,
                                         {{
                                             {1, RCC_APB1_DIV1},
                                             std::make_pair(2, RCC_APB1_DIV2),
                                             std::make_pair(4, RCC_APB1_DIV4),
                                             std::make_pair(8, RCC_APB1_DIV8),
                                             std::make_pair(16, RCC_APB1_DIV16),
                                         }});
}

[[nodiscard]] consteval uint32_t GetApb2Divider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 5>(div,
                                         {{
                                             std::make_pair(1, RCC_APB2_DIV1),
                                             std::make_pair(2, RCC_APB2_DIV2),
                                             std::make_pair(4, RCC_APB2_DIV4),
                                             std::make_pair(8, RCC_APB2_DIV8),
                                             std::make_pair(16, RCC_APB2_DIV16),
                                         }});
}

[[nodiscard]] consteval uint32_t GetApb3Divider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 5>(div,
                                         {{
                                             std::make_pair(1, RCC_APB3_DIV1),
                                             std::make_pair(2, RCC_APB3_DIV2),
                                             std::make_pair(4, RCC_APB3_DIV4),
                                             std::make_pair(8, RCC_APB3_DIV8),
                                             std::make_pair(16, RCC_APB3_DIV16),
                                         }});
}

[[nodiscard]] consteval uint32_t GetApb4Divider(uint32_t div) noexcept {
  return ct::StaticMap<int, uint32_t, 5>(div,
                                         {{
                                             std::make_pair(1, RCC_APB4_DIV1),
                                             std::make_pair(2, RCC_APB4_DIV2),
                                             std::make_pair(4, RCC_APB4_DIV4),
                                             std::make_pair(8, RCC_APB4_DIV8),
                                             std::make_pair(16, RCC_APB4_DIV16),
                                         }});
}

}   // namespace detail

template <ClockSettings CS>
bool ConfigureClocks() noexcept {
  using namespace detail;

  static_assert(CS.Validate());

  RCC_OscInitTypeDef osc_init{
      .OscillatorType      = RCC_OSCILLATORTYPE_HSI,
      .HSIState            = RCC_HSI_DIV1,
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
                   | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1,
      .SYSCLKSource = static_cast<uint32_t>(CS.sysclk_source),
      .SYSCLKDivider =
          GetSysClkDivider(CS.system_clock_settings.d1_core_prescaler),
      .AHBCLKDivider = GetAhbDivider(CS.system_clock_settings.d1_ahb_prescaler),
      .APB3CLKDivider =
          GetApb1Divider(CS.system_clock_settings.d1_apb3_prescaler),
      .APB1CLKDivider =
          GetApb1Divider(CS.system_clock_settings.d2_apb1_prescaler),
      .APB2CLKDivider =
          GetApb2Divider(CS.system_clock_settings.d2_apb2_prescaler),
      .APB4CLKDivider =
          GetApb2Divider(CS.system_clock_settings.d3_apb4_prescaler),
  };

  if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_4) != HAL_OK) {
    return false;
  }

  // Configure SPI1/2/3 clock
  if constexpr (CS.peripherals.spi123 != DefaultSpi123ClkSource) {
    switch (CS.peripherals.spi123) {
    case Spi123ClkSource::Pll1Q:
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL1_DIVQ);
      break;
    case Spi123ClkSource::Pll2P:
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL2_DIVP);
      break;
    case Spi123ClkSource::Pll3P:
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL3_DIVP);
      break;
    }
    LL_RCC_SetSPIClockSource(static_cast<uint32_t>(CS.peripherals.spi123));
  }

  // Configure SPI4/5 clock
  if constexpr (CS.peripherals.spi45 != DefaultSpi45ClkSource) {
    switch (CS.peripherals.spi45) {
    case Spi45ClkSource::Pll2Q:
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL2_DIVQ);
      break;
    case Spi45ClkSource::Pll3R:
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL3_DIVR);
      break;
    }
    LL_RCC_SetSPIClockSource(static_cast<uint32_t>(CS.peripherals.spi45));
  }

  // Configure SPI6 clock
  if constexpr (CS.peripherals.spi6 != DefaultSpi6ClkSource) {
    switch (CS.peripherals.spi6) {
    case Spi6ClkSource::Pll2Q: __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL2_DIVQ); break;
    case Spi6ClkSource::Pll3Q: __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL3_DIVQ); break;
    }
    LL_RCC_SetSPIClockSource(static_cast<uint32_t>(CS.peripherals.spi6));
  }

  return true;
}   // namespace stm32h7

static_assert(DefaultSystemClockSettings.Validate(64_MHz));

}   // namespace stm32h7