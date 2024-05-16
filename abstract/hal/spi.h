#pragma once

#include <concepts>
#include <cstdint>
#include <span>

namespace hal {

enum class SpiMode { Master, Slave };

enum class SpiTransmissionType { FullDuplex, HalfDuplex, TxOnly, RxOnly };

template <typename Impl>
concept SpiBase = requires {
  // SPI parameters
  { Impl::Mode } -> std::convertible_to<SpiMode>;
  { Impl::TransmissionType } -> std::convertible_to<SpiTransmissionType>;
  { Impl::DataSize } -> std::convertible_to<unsigned>;

  // SPI data size
  requires std::is_unsigned_v<typename Impl::Data>;
  requires std::numeric_limits<typename Impl::Data>::digits <= Impl::DataSize;
};

template<typename Impl>
concept SpiMaster = SpiBase<Impl> && (Impl::Mode == SpiMode::Master);

template <typename Impl>
concept BlockingRxSpiMaster = SpiMaster<Impl> && requires(Impl& impl) {
  requires Impl::TransmissionType != SpiTransmissionType::TxOnly;

  {
    impl.ReceiveBlocking(std::declval<std::span<typename Impl::Data>>(),
                         std::declval<uint32_t>())
  } -> std::convertible_to<bool>;
};

}   // namespace hal