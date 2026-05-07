module;

#include <concepts>
#include <cstdint>
#include <limits>
#include <span>

export module seq.hal:uart;

import seq.abstract;

import :common;

namespace seq::hal {

/** @brief UART event types. */
export enum class UartEventType : uint8_t {
  RxComplete = 1,
  TxComplete = 2,
};

/**
 * @brief UART event ID for a given UART instance and UART event.
 * @tparam Inst UART instance.
 * @tparam Event UART event type.
 */
template <PeripheralId auto Inst, UartEventType Event>
  requires(static_cast<unsigned>(Inst) <= std::numeric_limits<uint8_t>::max())
inline constexpr uint16_t UartEventId =
    (static_cast<uint16_t>(Inst) << 8U) | (static_cast<uint16_t>(Event));

/**
 * @brief UART event.
 * @tparam Inst UART instance.
 * @tparam Event UART event type.
 * @tparam Data Event associated data.
 */
template <auto Inst, UartEventType Event, concepts::EventData Data = void>
using UartEvent = ::seq::Event<PkgId, ModuleId::Uart, UartEventId<Inst, Event>, Data>;

/**
 * @brief UART receive complete event.
 * @tparam Inst UART instance.
 */
export template <PeripheralId auto Inst>
using UartRxComplete = UartEvent<Inst, UartEventType::RxComplete, uint32_t>;

/**
 * @brief UART transmit complete event.
 * @tparam Inst UART instance.
 */
export template <PeripheralId auto Inst>
using UartTxComplete = UartEvent<Inst, UartEventType::TxComplete>;

/**
 * UART transmit complete callback handler that emits a \c TxComplete event.
 * @tparam Inst UART instance.
 * @tparam ES Event sink.
 */
export template <PeripheralId auto Inst, concepts::EventSink ES>
class UartEmitTxEvent {
 public:
  constexpr void UartTransmitCallback() {
    //
    UartTxComplete<Inst>::template Emit<ES>();
  }
};

/**
 * UART receive complete callback handler that emits a \c RxComplete event.
 * @tparam Inst UART instance.
 * @tparam ES Event sink.
 */
export template <PeripheralId auto Inst, concepts::EventSink ES>
class UartEmitRxEvent {
 public:
  /**
   * UART receive callback implementation
   * @param data Received data
   */
  constexpr void UartReceiveCallback(std::span<std::byte> data) noexcept {
    UartRxComplete<Inst>::template Emit<ES>(static_cast<uint32_t>(data.size_bytes()));
  }
};

}   // namespace seq::hal
