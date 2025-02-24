#pragma once

#include <hal/dma.h>

#include <stm32g0/dma.h>
#include <stm32g0/peripheral_ids.h>

namespace stm32g0 {

template <typename Impl>
concept PeriodElapsedCallback = requires(Impl impl) {
  { impl.PeriodElapsedCallback() };
};

template <TimId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using TimPeriodElapsedDma = DmaChannel<Id, TimDmaRequest::PeriodElapsed, Prio>;

namespace detail {

void EnableTimClock(TimId id);
void EnableTimInterrupt(TimId id, uint32_t prio) noexcept;
void DisableTimInterrupt(TimId id) noexcept;

}

}   // namespace stm32g0