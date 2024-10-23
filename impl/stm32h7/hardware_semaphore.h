#pragma once

#include <hal/callback.h>
#include <hal/peripheral.h>

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

namespace detail {

void InitializeHsemInterrupt() noexcept;

}

struct HsemInterruptConfig {
  bool keep_notification_active_after_irq = true;
};

template <typename Impl, uint32_t I, HsemInterruptConfig C = {}>
  requires(I < 32)
class HsemInterruptImpl
    : public hal::UsedPeripheral
    , public HardwareSemaphore<I> {
 public:
  static constexpr uint32_t Id     = I;
  static constexpr uint32_t IdMask = 0b1U << I;
  static constexpr auto     Config = C;

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  void RegisterFreeCallback(hal::Callback<>& callback) {
    free_callback = &callback;
  }

  void FreeCallback() noexcept {
    if constexpr (Config.keep_notification_active_after_irq) {
      this->EnableNotification();
    }
    if (free_callback != nullptr) {
      (*free_callback)();
    }
  }

 protected:
  HsemInterruptImpl() noexcept { detail::InitializeHsemInterrupt(); }

 private:
  hal::Callback<>* free_callback{nullptr};
};

template <uint32_t I>
class HsemInterrupt : public hal::UnusedPeripheral<HsemInterrupt<I>> {
  friend void ::HAL_HSEM_FreeCallback(uint32_t sem_mask);

 public:
  static constexpr uint32_t Id     = I;
  static constexpr uint32_t IdMask = 0b1U << I;

  constexpr void FreeCallback() noexcept {}
};

}   // namespace stm32h7