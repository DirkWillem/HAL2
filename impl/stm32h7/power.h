#ifndef STM32H7_CMAKE_TEST_POWER_H
#define STM32H7_CMAKE_TEST_POWER_H

#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <stm32h7xx_hal.h>

#include "clocks.h"

namespace stm32h7 {

enum class PowerScale : uint32_t  {
  Vos0 = PWR_REGULATOR_VOLTAGE_SCALE0,
  Vos1 = PWR_REGULATOR_VOLTAGE_SCALE1,
  Vos2 = PWR_REGULATOR_VOLTAGE_SCALE2,
  Vos3 = PWR_REGULATOR_VOLTAGE_SCALE3,
};

enum class CoreVoltageSource : uint32_t  {
  Smps = PWR_DIRECT_SMPS_SUPPLY,
  Ldo  = PWR_LDO_SUPPLY,
};

struct PowerSupplySettings {
  CoreVoltageSource vcore_source;
  PowerScale        power_scale;

  template <ClockSettings CS>
  [[nodiscard]] consteval bool Validate() const noexcept {
    constexpr ct::Frequency auto cpu1_frequency =
        CS.system_clock_settings.Cpu1ClockFrequency(
            CS.SysClkSourceClockFrequency());
#if defined(STM32H755)
    switch (power_scale) {
    case PowerScale::Vos0:
      assert(("SMPS cannot be used with VOS0 power scale",
              vcore_source != CoreVoltageSource::Smps));
      assert(("Maximum frequency for VOS0/LDO is 480 MHz",
              cpu1_frequency <= 480_MHz));
      break;
    case PowerScale::Vos1:
      assert(("Maximum frequency for LDO/VOS1 is 400 MHz",
              cpu1_frequency <= 400_MHz));
      break;
    case PowerScale::Vos2:
      assert(("Maximum frequency for LDO/VOS2 is 400 MHz",
              cpu1_frequency <= 300_MHz));
      break;
    case PowerScale::Vos3:
      assert(("Maximum frequency for LDO/VOS3 is 200 MHz",
              cpu1_frequency <= 300_MHz));
      break;
    }
#else
#error "Implementation of power settings for this STM32H7 variant"
#endif

    return true;
  }
};

template <ClockSettings CS, PowerSupplySettings PSS>
void ConfigurePowerSupply() noexcept {
  static_assert(PSS.template Validate<CS>());

  HAL_PWREx_ConfigSupply(static_cast<uint32_t>(PSS.vcore_source));
  __HAL_PWR_VOLTAGESCALING_CONFIG(static_cast<uint32_t>(PSS.power_scale));

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
}

}   // namespace stm32h7

#endif   // STM32H7_CMAKE_TEST_POWER_H
