#pragma once

#include <cstdint>
#include <cstring>

#include <halstd/logic.h>

#include <vrpc.h>

#include "proto_helpers.h"

#include <sc/statechart.h>
#include <sc/statechart_callback.h>

namespace vrpc {

struct IdleA {};
struct ReadA {};
struct WriteB {};
struct ReadAWriteB {};
struct ReadANewB {};
struct IdleB {};
struct ReadB {};
struct WriteA {};
struct ReadBWriteA {};
struct ReadBNewA {};

struct StartRead {};
struct EndRead {};
struct StartWrite {};
struct EndWrite {};

template <typename Sys, typename T>
class ParameterSlot {
  using States = sc::States<IdleA, ReadA, WriteB, ReadAWriteB, ReadANewB, IdleB,
                            ReadB, WriteA, ReadBWriteA, ReadBNewA>;
  using Events = sc::Events<StartRead, EndRead, StartWrite, EndWrite>;

  static constexpr auto CreateStateChart() {
    return sc::StateChartRunner{
        halstd::Marker<Sys>(),
        sc::StateChart<States, Events>::Chart{
            IdleA{},
            sc::Transitions{
                sc::MakeTransition<IdleA, StartRead, ReadA>(),
                sc::MakeTransition<IdleA, StartWrite, WriteB>(),

                sc::MakeTransition<ReadA, EndRead, IdleA>(),
                sc::MakeTransition<ReadA, StartWrite, ReadAWriteB>(),

                sc::MakeTransition<WriteB, StartRead, ReadAWriteB>(),
                sc::MakeTransition<WriteB, EndWrite, IdleB>(),

                sc::MakeTransition<ReadAWriteB, EndRead, WriteB>(),
                sc::MakeTransition<ReadAWriteB, EndWrite, ReadANewB>(),

                sc::MakeTransition<ReadANewB, EndRead, IdleB>(),

                sc::MakeTransition<IdleB, StartRead, ReadB>(),
                sc::MakeTransition<IdleB, StartWrite, WriteA>(),

                sc::MakeTransition<ReadB, EndRead, IdleB>(),
                sc::MakeTransition<ReadB, StartWrite, ReadBWriteA>(),

                sc::MakeTransition<WriteA, StartRead, ReadBWriteA>(),
                sc::MakeTransition<WriteA, EndWrite, IdleA>(),

                sc::MakeTransition<ReadBWriteA, EndRead, WriteA>(),
                sc::MakeTransition<ReadBWriteA, EndWrite, ReadBNewA>(),

                sc::MakeTransition<ReadBNewA, EndRead, IdleA>(),
            },
        }};
  }

  using StateChart = std::decay_t<decltype(CreateStateChart())>;

 public:
  template <std::invocable<const T&> RA>
    requires(!std::is_same_v<
             decltype(std::declval<RA>()(std::declval<const T&>())), void>)
  std::optional<decltype(std::declval<RA>()(std::declval<const T&>()))>
  Read(RA read_action) noexcept {
    if (state_chart.ApplyEvent(StartRead{})) {
      const auto* read_ptr = GetReadPtr();
      if (read_ptr == nullptr) {
        return {};
      }

      const auto& read_val = *read_ptr;
      const auto  result   = read_action(read_val);
      if (!state_chart.ApplyEvent(EndRead{})) {
        return {};
      }

      return result;
    }

    return {};
  }

  template <std::invocable<const T&> RA>
    requires(std::is_same_v<
             decltype(std::declval<RA>()(std::declval<const T&>())), void>)
  bool Read(RA read_action) noexcept {
    if (state_chart.ApplyEvent(StartRead{})) {
      const auto* read_ptr = GetReadPtr();
      if (read_ptr == nullptr) {
        return false;
      }

      const auto& read_val = *read_ptr;
      read_action(read_val);
      if (!state_chart.ApplyEvent(EndRead{})) {
        return false;
      }

      return true;
    }

    return false;
  }

  template <std::invocable<T&> WA>
  bool Write(WA write_action) noexcept {
    if (state_chart.ApplyEvent(StartWrite{})) {
      auto* write_ptr = GetWritePtr();
      if (write_ptr == nullptr) {
        return false;
      }

      if (!Read([write_ptr](const auto& val) {
            std::memcpy(write_ptr, &val, sizeof(T));
          })) {
        return false;
      }

      write_action(*write_ptr);
      return state_chart.ApplyEvent(EndWrite{});
    }

    return {};
  }

  template <std::invocable<T&, const halstd::Callback<>> WA>
  bool WriteAsync(WA write_action) noexcept {
    if (state_chart.ApplyEvent(StartWrite{})) {
      const auto* cur_val   = GetReadPtr();
      auto*       write_ptr = GetWritePtr();

      if (cur_val == nullptr || write_ptr == nullptr) {
        return false;
      }

      std::memcpy(write_ptr, cur_val, sizeof(T));

      write_action(*write_ptr, end_write_callback);
      return true;
    }

    return false;
  }

 private:
  const T* GetReadPtr() const noexcept {
    if (state_chart.Visit(sc::IsInState<ReadA>())
        || state_chart.Visit(sc::IsInState<ReadAWriteB>())
        || state_chart.Visit(sc::IsInState<ReadANewB>())) {
      return &value_a;
    }

    if (state_chart.Visit(sc::IsInState<ReadB>())
        || state_chart.Visit(sc::IsInState<ReadBWriteA>())
        || state_chart.Visit(sc::IsInState<ReadBNewA>())) {
      return &value_b;
    }

    return nullptr;
  }

  T* GetWritePtr() noexcept {
    if (state_chart.Visit(sc::IsInState<WriteA>())
        || state_chart.Visit(sc::IsInState<ReadBWriteA>())) {
      return &value_a;
    }

    if (state_chart.Visit(sc::IsInState<WriteB>())
        || state_chart.Visit(sc::IsInState<ReadAWriteB>())) {
      return &value_b;
    }

    return nullptr;
  }

  StateChart state_chart{CreateStateChart()};
  T          value_a{};
  T          value_b{};

  sc::ApplyEventCallback<StateChart, EndWrite> end_write_callback{state_chart};
};

template <typename PMsg, typename WMsg,
          halstd::FieldPointer<WMsg, uint32_t*> auto WF,
          halstd::FieldPointer<WMsg, PMsg> auto      WB>
struct ParameterMapping {
  static void ApplyWriteMessage(const WMsg& write_msg, PMsg& param_msg) {
    const auto written_fields_opt =
        GetRepeatedFieldFromPtr<WMsg, uint32_t>(write_msg, WF);
    const auto body = write_msg.*WB;

    if (!written_fields_opt.has_value()) {
      return;
    }

    for (const auto field : *written_fields_opt) {
      pb_field_iter_t src_iter{};
      pb_field_iter_t dst_iter{};

      const auto fields = nanopb::MessageDescriptor<PMsg>::fields();
      pb_field_iter_begin_const(&src_iter, fields, &body);
      pb_field_iter_begin(&dst_iter, fields, &param_msg);

      if (pb_field_iter_find(&src_iter, field)
          && pb_field_iter_find(&dst_iter, field)) {
        std::memcpy(dst_iter.pData, src_iter.pData,
                    std::min(src_iter.data_size, dst_iter.data_size));
      }
    }
  }
};

template <typename Impl>
concept ParameterStorage = requires(Impl& impl, const Impl& cimpl) {
  impl.template Write<halstd::Empty>(std::declval<uint32_t>(),
                                     std::declval<const halstd::Empty&>());
  impl.template Read<halstd::Empty>(std::declval<uint32_t>(),
                                    std::declval<halstd::Empty&>());

  { cimpl.Contains(std::declval<uint32_t>()) } -> std::convertible_to<bool>;
};

class InMemoryStorage {
 public:
  template <typename T>
  void Write(uint32_t, const T&) noexcept {}

  template <typename T>
  void Read(uint32_t, T&) noexcept {}

  [[nodiscard]] constexpr bool Contains(uint32_t) const noexcept {
    return false;
  }
};

static_assert(ParameterStorage<InMemoryStorage>);

}   // namespace vrpc