#pragma once

namespace hal {

enum class SpiMode { Master, Slave };

enum class SpiTransmissionType { FullDuplex, HalfDuplex, TxOnly, RxOnly };

}   // namespace hal