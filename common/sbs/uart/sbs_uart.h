#pragma once

#include <atomic>
#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>

#include <constexpr_tools/buffer_io.h>
#include <constexpr_tools/crc.h>
#include <constexpr_tools/math.h>
#include <constexpr_tools/type_helpers.h>

#include <hal/clocks.h>

#include <sbs/sbs.h>
#include <sbs/sbs_type_name.h>

#include <hal/uart.h>

#include "detail/slot.h"

namespace sbs::uart {

namespace detail {

template <typename T>
struct FrameHelper;

template <auto I, SignalDescriptor... Sts>
struct FrameHelper<Frame<I, Sts...>> {
  static constexpr auto SignalNames =
      std::array<std::string_view, sizeof...(Sts)>{Sts::Name...};

  static constexpr std::array<std::string_view, sizeof...(Sts)> SignalTypeNames{
      {TypeName<typename Sts::Type>...}};
};

}   // namespace detail

inline constexpr auto FrameStartChar = std::byte{0xBB};
inline constexpr auto FrameEndChar   = std::byte{0xEE};

inline constexpr auto SignalStartChar = std::byte{'s'};
inline constexpr auto SignalEndChar   = std::byte{'S'};

inline constexpr auto DisableFrameStartChar = std::byte{'d'};
inline constexpr auto DisableFrameEndChar   = std::byte{'D'};

inline constexpr auto EnableFrameStartChar = std::byte{'e'};
inline constexpr auto EnableFrameEndChar   = std::byte{'E'};

inline constexpr auto ListFramesStartChar = std::byte{'l'};
inline constexpr auto ListFramesEndChar   = std::byte{'L'};

inline constexpr auto DescribeFrameStartChar = std::byte{'i'};
inline constexpr auto DescribeFrameEndChar   = std::byte{'I'};

inline constexpr auto NullFrameStartChar = std::byte{'('};
inline constexpr auto NullFrameEndChar   = std::byte{')'};

template <hal::AsyncUart Uart, hal::Clock C, FrameType... Frames>
  requires(sizeof...(Frames) > 0)
/**
 * UART Implementation for SBS (Simple Binary Signals)
 * @tparam Uart UART implementation
 * @tparam Signals Signals that are transmitted
 */
class SbsUart : private detail::FrameSlot<typename Frames::FrameType>... {
  enum class State { Idle, Sending };
  enum class Command {
    None,
    ListFrames,
    DescribeFrame,
    EnableFrame,
    DisableFrame,
    SendNullFrame,
  };

  static constexpr auto MaxFrameSize =
      2 * sizeof(std::byte) + 3 * sizeof(uint32_t)
      + ct::Max(
          std::array<std::size_t, sizeof...(Frames)>{Frames::PayloadSize...});

  consteval static std::size_t ListFramesResponseSize() {
    std::array<std::string_view, sizeof...(Frames)> frame_names{
        Frames::Name...};

    std::size_t size = 2 * sizeof(std::byte) + sizeof(uint32_t);
    for (auto frame_name : frame_names) {
      size += frame_name.length() + 5;
    }

    return size;
  }

  template <FrameType Frame>
  consteval static std::size_t FrameInfoResponseSize() noexcept {
    std::size_t size = 3 * sizeof(std::byte) + sizeof(uint32_t);

    for (const auto frame_name :
         detail::FrameHelper<typename Frame::FrameType>::SignalNames) {
      size += 1 + frame_name.size();
    }

    for (const auto type_name :
         detail::FrameHelper<typename Frame::FrameType>::SignalTypeNames) {
      size += 1 + type_name.size();
    }

    return size;
  }

  static constexpr auto MaxFrameInfoResponsesSize =
      ct::Max(std::array<std::size_t, sizeof...(Frames)>{
          FrameInfoResponseSize<Frames>()...});

  static constexpr auto MaxTxPayloadSize = std::max(
      {MaxFrameSize, ListFramesResponseSize(), MaxFrameInfoResponsesSize});
  static constexpr auto TxBufSize = 11 + MaxTxPayloadSize;

 public:
  /**
   * Constructor
   * @param uart Reference to UART instance
   */
  explicit SbsUart(Uart& uart) noexcept
      : uart{uart}
      , uart_rx_callback{this, &SbsUart::UartReceiveCallback}
      , uart_tx_callback{this, &SbsUart::UartTransmitCallback} {
    uart.RegisterUartTransmitCallback(uart_tx_callback);
    uart.RegisterUartReceiveCallback(uart_rx_callback);

    uart.Receive(rx_buffer);
  }

  ~SbsUart() override = default;

  template <FrameType F>
    requires std::disjunction_v<std::is_same<F, Frames>...>
  /**
   * Writes a signal to the UART
   * @tparam F Frame to send
   * @param values Frame values
   * @return Whether writing the signal was successful
   */
  bool WriteSignal(const typename F::SignalsTuple& values) noexcept {
    if (!detail::FrameSlot<typename F::FrameType>::Write(values)) {
      return false;
    }

    TrySendFrame();

    return true;
  }

  /**
   * Sends a null frame over UART
   * @return Whether the null frame was sent
   */
  bool SendNullFrame() noexcept {
    auto current_command = Command::None;
    if (pending_command.compare_exchange_strong(current_command,
                                                Command::SendNullFrame,
                                                std::memory_order::seq_cst)) {
      TrySendFrame();
      return true;
    } else {
      return false;
    }
  }

 private:
  void UartReceiveCallback(std::span<std::byte> data) {
    if (data.empty()) {
      uart.Receive(rx_buffer);
      return;
    }

    rx_message_view = data;

    auto incoming_command = Command::None;

    switch (rx_message_view[0]) {
    case ListFramesStartChar: incoming_command = Command::ListFrames; break;
    case DescribeFrameStartChar:
      incoming_command = Command::DescribeFrame;
      break;
    case EnableFrameStartChar: incoming_command = Command::EnableFrame; break;
    case DisableFrameStartChar: incoming_command = Command::DisableFrame; break;
    default: break;
    }

    if (incoming_command != Command::None) {
      pending_command.store(incoming_command, std::memory_order::seq_cst);
      TrySendFrame();
    }
  }

  void UartTransmitCallback() noexcept { TrySendNextFrame(); }

  void TrySendFrame() noexcept {
    State state_expect{State::Idle};
    if (!state.compare_exchange_strong(state_expect, State::Sending,
                                       std::memory_order::acquire)) {
      return;
    }

    TrySendNextFrame();
  }

  void TrySendNextFrame() noexcept {
    auto cmd = pending_command.load(std::memory_order::seq_cst);
    if (cmd != Command::None) {
      pending_command.store(Command::None, std::memory_order::seq_cst);
      HandleCommand(cmd);
      uart.Receive(rx_buffer);
      return;
    }

    const auto frame_sent = (... || TrySendFrame<Frames>());

    if (!frame_sent) {
      state.store(State::Idle, std::memory_order::release);
    }
  }

  void HandleCommand(Command cmd) {
    switch (cmd) {
    case Command::ListFrames: ListFrames(); break;
    case Command::DescribeFrame: DescribeFrame(); break;
    case Command::EnableFrame: EnableFrame(); break;
    case Command::DisableFrame: DisableFrame(); break;
    case Command::SendNullFrame: HandleSendNullFrame(); break;
    }
  }

  void HandleSendNullFrame() noexcept {
    ct::BufferWriter buf_writer{tx_payload};
    buf_writer.Write(NullFrameStartChar);
    buf_writer.Write(NullFrameEndChar);
    if (buf_writer.valid()) {
      SendPayload(buf_writer.WrittenData());
    }
  }

  void ListFrames() noexcept {
    // Parse command
    ct::BufferReader buf_reader{rx_message_view};
    buf_reader.ReadLiteral(ListFramesStartChar);
    buf_reader.ReadLiteral(ListFramesEndChar);
    if (!buf_reader.valid()) {
      return;
    }

    // Form response
    std::array<std::tuple<uint32_t, std::string_view>, sizeof...(Frames)>
        frame_info{{std::make_tuple(
            static_cast<uint32_t>(Frames::FrameType::Id), Frames::Name)...}};

    ct::BufferWriter buf_writer{tx_payload};
    buf_writer.Write(ListFramesStartChar);
    buf_writer.Write(static_cast<uint32_t>(sizeof...(Frames)));

    for (const auto [frame_id, frame_name] : frame_info) {
      buf_writer.Write(frame_id);
      buf_writer.Write(static_cast<uint8_t>(frame_name.length()));
      buf_writer.WriteString(frame_name);
    }
    buf_writer.Write(ListFramesEndChar);

    if (buf_writer.valid()) {
      SendPayload(buf_writer.WrittenData());
    }
  }

  void DescribeFrame() noexcept {
    // Parse command
    uint32_t         frame_id{0};
    ct::BufferReader r{rx_message_view};
    r.ReadLiteral(DescribeFrameStartChar);
    r.Read(frame_id);
    r.ReadLiteral(DescribeFrameEndChar);

    if (!r.valid()) {
      return;
    }

    // Handle command
    const auto generic_slots = generic_slot_pointers();

    ct::BufferWriter w(tx_payload);
    w.Write(DescribeFrameStartChar);

    bool found = false;
    for (const auto* slot : generic_slots) {
      if (slot->id == frame_id) {
        slot->Describe(w);
        found = true;
        break;
      }
    }
    w.Write(DescribeFrameEndChar);

    if (found && w.valid()) {
      SendPayload(w.WrittenData());
    }
  };

  void EnableFrame() noexcept {
    // Parse command
    uint32_t         frame_id{0};
    ct::BufferReader r{rx_message_view};
    r.ReadLiteral(EnableFrameStartChar);
    r.Read(frame_id);
    r.ReadLiteral(EnableFrameEndChar);

    if (!r.valid()) {
      return;
    }

    // Handle command
    auto generic_slots = generic_slot_pointers();

    bool found = false;
    for (auto* slot : generic_slot_pointers()) {
      if (slot->id == frame_id) {
        slot->Enable();
        found = true;
        break;
      }
    }

    if (found) {
      ct::BufferWriter w{tx_payload};
      w.Write(EnableFrameStartChar);
      w.Write(EnableFrameEndChar);

      if (w.valid()) {
        SendPayload(w.WrittenData());
      }
    }
  }

  void DisableFrame() noexcept {
    // Parse command
    uint32_t         frame_id{0};
    ct::BufferReader r{rx_message_view};
    r.ReadLiteral(DisableFrameStartChar);
    r.Read(frame_id);
    r.ReadLiteral(DisableFrameEndChar);

    if (!r.valid()) {
      return;
    }

    // Handle command
    auto generic_slots = generic_slot_pointers();

    bool found = false;
    for (auto* slot : generic_slot_pointers()) {
      if (slot->id == frame_id) {
        slot->Disable();
        found = true;
        break;
      }
    }

    if (found) {
      ct::BufferWriter w{tx_payload};
      w.Write(DisableFrameStartChar);
      w.Write(DisableFrameEndChar);

      if (w.valid()) {
        SendPayload(w.WrittenData());
      }
    }
  }

  template <FrameType FT>
  bool TrySendFrame() noexcept {
    return detail::FrameSlot<typename FT::FrameType>::Read(
        [this](const typename FT::SignalsTuple& data) {
          // Calculate frame size
          constexpr auto FrameSize = 2 * sizeof(std::byte)
                                     + sizeof(typename FT::FrameIdType)
                                     + 2 * sizeof(uint32_t) + FT::PayloadSize;

          // Write start character, stop character and frame ID
          tx_payload[0]             = SignalStartChar;
          tx_payload[FrameSize - 1] = SignalEndChar;
          std::memcpy(&tx_payload[1], &FT::Id, sizeof(FT::Id));

          // Write timestamp
          uint32_t timestamp = std::chrono::duration_cast<
                                   std::chrono::duration<uint32_t, std::milli>>(
                                   C::TimeSinceBoot())
                                   .count();
          std::memcpy(&tx_payload[5], &timestamp, sizeof(timestamp));

          // Write payload size
          auto size = static_cast<uint32_t>(FT::PayloadSize);
          std::memcpy(&tx_payload[9], &size, sizeof(size));

          // Write frame payload
          auto payload_buf =
              std::span{&tx_payload[sizeof(FT::Id) + 2 * sizeof(uint32_t) + 1],
                        FT::PayloadSize};
          EncodeSignals(data, payload_buf,
                        std::make_index_sequence<FT::NumSignals>());

          // Send data
          SendPayload(std::span{tx_payload.data(), FrameSize});
        });
  }

  template <SignalType... STs, std::size_t... Idxs>
  static void
  EncodeSignals(const std::tuple<STs...>& data, std::span<std::byte> into,
                [[maybe_unused]] std::index_sequence<Idxs...>) noexcept {
    (..., EncodeSignal(std::get<Idxs>(data), into));
  }

  template <SignalType ST>
  static void EncodeSignal(ST value, std::span<std::byte>& into) noexcept {
    std::memcpy(into.data(), &value, sizeof(value));
    into = into.subspan(sizeof(ST));
  }

  void SendPayload(std::span<std::byte> payload_view) {
    ct::BufferWriter start_writer{std::span{tx_buffer}.subspan(0, 8)};
    start_writer.Write(static_cast<uint32_t>(0xBBBBBBBB));
    start_writer.Write(static_cast<uint32_t>(payload_view.size()));

    ct::BufferWriter end_writer(
        std::span{tx_buffer}.subspan(payload_view.size() + 8));
    end_writer.Write(ct::Crc16(payload_view));
    end_writer.Write(FrameEndChar);

    uart.Write(std::span{tx_buffer}.subspan(0, payload_view.size() + 11));
  }

  [[nodiscard]] inline std::array<detail::GenericFrameSlot*, sizeof...(Frames)>
  generic_slot_pointers() noexcept {
    return std::array<detail::GenericFrameSlot*, sizeof...(Frames)>{
        static_cast<detail::GenericFrameSlot*>(
            static_cast<detail::FrameSlot<typename Frames::FrameType>*>(
                this))...};
  }

  [[nodiscard]] inline std::array<const detail::GenericFrameSlot*,
                                  sizeof...(Frames)>
  generic_slot_pointers() const noexcept {
    return std::array<const detail::GenericFrameSlot*, sizeof...(Frames)>{
        static_cast<const detail::GenericFrameSlot*>(
            static_cast<const detail::FrameSlot<typename Frames::FrameType>*>(
                this))...};
  }

  Uart&                uart;
  std::atomic<State>   state{State::Idle};
  std::atomic<Command> pending_command{Command::None};

  std::array<std::byte, 128>       rx_buffer{};
  std::array<std::byte, TxBufSize> tx_buffer{};
  std::span<std::byte>             tx_payload{&tx_buffer[8], MaxTxPayloadSize};

  std::span<std::byte> rx_message_view{};

  hal::MethodCallback<SbsUart, std::span<std::byte>> uart_rx_callback;
  hal::MethodCallback<SbsUart>                       uart_tx_callback;
};

}   // namespace sbs::uart