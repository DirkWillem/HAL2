#pragma once

#include <hal/uart.h>

namespace sbs::uart::detail {

enum class SlotState { Disabled, Empty, Writing, Ready, Reading };

class GenericFrameSlot {
 public:
  virtual ~GenericFrameSlot() noexcept = default;

  virtual void Describe(ct::BufferWriter& w) const noexcept = 0;

  void Enable() noexcept {
    state.store(SlotState::Empty, std::memory_order::seq_cst);
  }

  void Disable() noexcept {
    state.store(SlotState::Disabled, std::memory_order::seq_cst);
  }

  uint32_t               id{0};
  std::atomic<SlotState> state{SlotState::Disabled};

 protected:
  explicit GenericFrameSlot(uint32_t id) noexcept
      : id{id} {}
};

template <typename F>
class FrameSlot;

template <auto FId, SignalDescriptor... Signals>
class FrameSlot<Frame<FId, Signals...>> : public GenericFrameSlot {
  using FrameType = Frame<FId, Signals...>;

 protected:
  FrameSlot()
      : GenericFrameSlot{static_cast<uint32_t>(FId)} {}

  ~FrameSlot() override = default;

  [[nodiscard]] inline bool
  Write(const typename FrameType::SignalsTuple& values) noexcept {
    // Check if the slot is empty or ready
    SlotState current_state{SlotState::Empty};
    if (!state.compare_exchange_strong(current_state, SlotState::Writing,
                                       std::memory_order::acquire)) {
      current_state = SlotState::Ready;
      if (!state.compare_exchange_strong(current_state, SlotState::Writing,
                                         std::memory_order::acquire)) {
        return false;
      }
    }

    // Write the data to the slot
    data = values;

    // Set the slot state to Ready
    state.store(SlotState::Ready, std::memory_order::release);
    return true;
  }

  [[nodiscard]] inline bool
  Read(std::invocable<const typename FrameType::SignalsTuple&> auto reader) {
    // Check if the slot is readable
    SlotState current_state{SlotState::Ready};
    if (!state.compare_exchange_strong(current_state, SlotState::Reading,
                                       std::memory_order::acquire)) {
      return false;
    }

    // Read the data
    reader(data);

    // Set the slot state to Empty
    state.store(SlotState::Empty, std::memory_order::release);
    return true;
  }

  void Describe(ct::BufferWriter& w) const noexcept final {
    std::array<std::tuple<std::string_view, std::string_view>,
               sizeof...(Signals)>
        signal_info{std::make_tuple(Signals::Name,
                                    TypeName<typename Signals::Type>)...};

    const auto disabled = state.load() == SlotState::Disabled;
    if (disabled) {
      w.Write(std::byte{0x00});
    } else {
      w.Write(std::byte{0x01});
    }

    w.Write(static_cast<uint32_t>(signal_info.size()));
    for (const auto [signal_name, type_name] : signal_info) {
      w.Write(static_cast<uint8_t>(signal_name.size()));
      w.WriteString(signal_name);
      w.Write(static_cast<uint8_t>(type_name.size()));
      w.WriteString(type_name);
    }
  }

 private:
  typename FrameType::SignalsTuple data{};
};

}   // namespace sbs::uart::detail
