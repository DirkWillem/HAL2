#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <span>

#include "callback.h"

namespace hal {

enum class SpiMode { Master, Slave };

enum class SpiTransmissionType { FullDuplex, HalfDuplex, TxOnly, RxOnly };

[[nodiscard]] constexpr bool
SpiTransmitEnabled(SpiTransmissionType tt) noexcept {
  return tt != SpiTransmissionType::RxOnly;
}

[[nodiscard]] constexpr bool
SpiReceiveEnabled(SpiTransmissionType tt) noexcept {
  return tt != SpiTransmissionType::TxOnly;
}

template <typename Impl>
concept SpiBase = requires {
  // SPI parameters
  { Impl::Mode } -> std::convertible_to<SpiMode>;
  { Impl::TransmissionType } -> std::convertible_to<SpiTransmissionType>;
  { Impl::DataSize } -> std::convertible_to<unsigned>;

  // SPI data size
  requires std::is_same_v<typename Impl::Data, std::byte>
               || std::is_unsigned_v<typename Impl::Data>;
  requires std::numeric_limits<typename Impl::Data>::digits <= Impl::DataSize;
};

template <typename Impl>
concept SpiMaster = SpiBase<Impl> && (Impl::Mode == SpiMode::Master);

template <typename Impl>
concept BlockingRxSpiMaster = SpiMaster<Impl> && requires(Impl& impl) {
  requires Impl::TransmissionType != SpiTransmissionType::TxOnly;

  {
    impl.ReceiveBlocking(std::declval<std::span<typename Impl::Data>>(),
                         std::declval<std::chrono::milliseconds>())
  } -> std::convertible_to<bool>;
};

template <typename Impl>
concept AsyncRxSpiMaster = SpiMaster<Impl> && requires(Impl& impl) {
  requires Impl::TransmissionType != SpiTransmissionType::TxOnly;

  impl.SpiReceiveCallback(std::declval<std::span<typename Impl::Data>>());

  {
    impl.Receive(std::declval<std::span<typename Impl::Data>>())
  } -> std::convertible_to<bool>;
};

template <typename Impl>
concept RegisterableSpiRxCallback = requires(Impl& impl) {
  requires std::is_same_v<typename Impl::RxData, std::byte>
               || std::is_unsigned_v<typename Impl::RxData>;

  impl.RegisterSpiRxCallback(
      std::declval<hal::Callback<std::span<typename Impl::RxData>>&>());
};

template <typename D>
class SpiRxCallback {
 public:
  using RxData = D;

  constexpr void RegisterSpiRxCallback(
      hal::Callback<std::span<RxData>>& new_callback) noexcept {
    rx_callback = &new_callback;
  }

  constexpr void SpiReceiveCallback(std::span<RxData> data) {
    if (rx_callback != nullptr) {
      (*rx_callback)(data);
    }
  }

 private:
  hal::Callback<std::span<RxData>>* rx_callback{nullptr};
};

static_assert(RegisterableSpiRxCallback<SpiRxCallback<uint8_t>>);
static_assert(RegisterableSpiRxCallback<SpiRxCallback<uint16_t>>);

template <typename Impl>
concept AsyncTxSpiMaster = SpiMaster<Impl> && requires(Impl& impl) {
  requires Impl::TransmissionType != SpiTransmissionType::RxOnly;

  impl.SpiTransmitCallback();

  {
    impl.Transmit(std::declval<std::span<typename Impl::Data>>())
  } -> std::convertible_to<bool>;
};

template <typename Impl>
concept RegisterableSpiTxCallback = requires(Impl& impl) {
  impl.RegisterSpiTxCallback(std::declval<hal::Callback<>&>());
};

class SpiTxCallback {
 public:
  constexpr void RegisterSpiTxCallback(hal::Callback<>& new_callback) noexcept {
    tx_callback = &new_callback;
  }

  constexpr void SpiTransmitCallback() {
    if (tx_callback != nullptr) {
      (*tx_callback)();
    }
  }

 private:
  hal::Callback<>* tx_callback{nullptr};
};

static_assert(RegisterableSpiTxCallback<SpiTxCallback>);

template <typename Impl>
concept AsyncDuplexSpiMaster = AsyncRxSpiMaster<Impl> && AsyncTxSpiMaster<Impl>;

}   // namespace hal