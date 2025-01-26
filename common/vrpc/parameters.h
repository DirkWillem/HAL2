#pragma once

#include <cstdint>
#include <cstring>

#include <halstd/logic.h>

#include <vrpc.h>

#include "proto_helpers.h"

namespace vrpc {

enum class ReaderState : uint8_t {
  Idle,
  ReadingA,
  ReadingB,
};

enum class WriterState : uint8_t {
  Clean,
  WritingA,
  WritingB,
  Dirty,
};

enum class WritingSlot : uint8_t { A, B };

struct SlotState {
  //   ReaderState reader{ReaderState::Idle};
  //   WriterState writer{WriterState::Clean};
  //   WritingSlot writing_slot{WritingSlot::A};
  //
  //   [[nodiscard]] constexpr uint32_t Raw() const noexcept {
  //     return static_cast<uint32_t>(reader) | (static_cast<uint32_t>(writer)
  //     << 8)
  //            | (static_cast<uint32_t>(writing_slot) << 16);
  //   }
  //
  //   static constexpr SlotState FromRaw(uint32_t raw) noexcept {
  //     return {
  //         .reader       = static_cast<ReaderState>(raw & 0xFF),
  //         .writer       = static_cast<WriterState>((raw >> 8) & 0xFF),
  //         .writing_slot = static_cast<WritingSlot>((raw >> 16) & 0xFF),
  //     };
  //   }
  //
  //   template <ReaderState R, WriterState W, WritingSlot WS>
  //   static constexpr SlotState Create() noexcept {
  //     static_assert(
  //         ct::Implies(R == ReaderState::ReadingB, WS == WritingSlot::A));
  //     static_assert(
  //         ct::Implies(R == ReaderState::ReadingA, WS == WritingSlot::B));
  //     static_assert(
  //         ct::Implies(W == WriterState::WritingA, WS == WritingSlot::A));
  //     static_assert(
  //         ct::Implies(W == WriterState::WritingB, WS == WritingSlot::B));
  //
  //     return {.reader = R, .writer = W, .writing_slot = WS};
  //   }
  // };
  //
  // template <typename T>
  // class ParameterSlot {
  //  public:
  //   T Read() {}
  //
  //  private:
  //   std::atomic<uint32_t> state{SlotState{}.Raw()};
  //   T                     a;
  //   T                     b;
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

}   // namespace vrpc