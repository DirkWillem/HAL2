#include "memory_config.h"

namespace stm32h7::detail {

void ConfigureMpuImpl(MpuControlMode             control_mode,
                      std::span<const MpuRegionConfig> regions) noexcept {
  HAL_MPU_Disable();

  for (const auto& region : regions) {
    MPU_Region_InitTypeDef region_init{
        .Enable           = MPU_REGION_ENABLE,
        .Number           = std::to_underlying(region.num),
        .BaseAddress      = region.base_addr,
        .Size             = std::to_underlying(region.size),
        .SubRegionDisable = 0,
        .TypeExtField     = std::to_underlying(region.tex),
        .AccessPermission = std::to_underlying(region.access),
        .DisableExec      = region.executable ? MPU_INSTRUCTION_ACCESS_ENABLE
                                              : MPU_INSTRUCTION_ACCESS_DISABLE,
        .IsShareable =
            region.shareable ? MPU_ACCESS_SHAREABLE : MPU_ACCESS_NOT_SHAREABLE,
        .IsCacheable =
            region.cacheable ? MPU_ACCESS_CACHEABLE : MPU_ACCESS_NOT_CACHEABLE,
        .IsBufferable = region.bufferable ? MPU_ACCESS_BUFFERABLE
                                          : MPU_ACCESS_NOT_BUFFERABLE,
    };

    HAL_MPU_ConfigRegion(&region_init);
  }

  HAL_MPU_Enable(std::to_underlying(control_mode));
}

}   // namespace stm32h7::detail