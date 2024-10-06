#pragma once

#include <stm32h7xx_hal.h>

#include "core.h"

namespace stm32h7 {

template <uint32_t Id>
class HardwareSemaphore {
 public:
  HardwareSemaphore() { __HAL_RCC_HSEM_CLK_ENABLE(); }

  void EnableNotification() noexcept {
    HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(Id));
  }

  void DisableNotification() noexcept {
    HAL_HSEM_DeactivateNotification(__HAL_HSEM_SEMID_TO_MASK(Id));
  }

  void EnterStopModeUntilNotified() noexcept {
    if constexpr (CurrentCore == Core::Cm7) {
      HAL_PWREx_ClearPendingEvent();
      HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE,
                              PWR_D1_DOMAIN);
      __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(Id));
    } else if constexpr (CurrentCore == Core::Cm4) {
      HAL_PWREx_ClearPendingEvent();
      HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE,
                              PWR_D2_DOMAIN);
      __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(Id));
    }
  }

  void NotifyOtherCore() noexcept {
    HAL_HSEM_FastTake(Id);
    HAL_HSEM_Release(Id, 0);
  }
};

}   // namespace stm32h7