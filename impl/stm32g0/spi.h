#pragma once

#include <stm32g0/dma.h>

namespace stm32g0 {

namespace detail {

void EnableSpiClk(SpiId id) noexcept;
void EnableSpiInterrupt(SpiId id) noexcept;

}   // namespace detail

template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiTxDma = DmaChannel<Id, SpiDmaRequest::Tx, Prio>;

template <SpiId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using SpiRxDma = DmaChannel<Id, SpiDmaRequest::Rx, Prio>;

}   // namespace stm32g0