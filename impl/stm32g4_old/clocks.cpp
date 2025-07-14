#include "clocks.h"

namespace stm32g4 {

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

bool ClockSettings::Configure() const noexcept {
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
      .SYSCLKSource   = static_cast<uint32_t>(system_clock_settings.sys_clk_source),
      .AHBCLKDivider  = GetHalAhbDivider(system_clock_settings.ahb_prescaler),
      .APB1CLKDivider = GetHalApbDivider(system_clock_settings.apb1_prescaler),
      .APB2CLKDivider = GetHalApbDivider(system_clock_settings.apb2_prescaler),
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

}   // namespace stm32g4