#include <math/geometry/transformation_matrix.h>
#include <math/linalg/matrix.h>

#include <pb_common.h>

#include <vrpc.h>

#include "generated/radiation_test_setup.h"

template <typename T, typename StructType, typename FieldType>
concept FieldPointer = requires(T ptr, StructType s) {
  { s.*ptr } -> std::convertible_to<FieldType>;
};

template <typename Msg, typename F>
static std::optional<pb_field_iter_t>
FindFieldByPointer(const Msg& msg, FieldPointer<Msg, F> auto field_ptr) {
  pb_field_iter_t iter{};
  pb_field_iter_begin_const(&iter, nanopb::MessageDescriptor<Msg>::fields(),
                            &msg);

  do {
    if (iter.pField == &(msg.*field_ptr)) {
      return {iter};
    }
  } while (pb_field_iter_next(&iter));

  return {};
}

template <typename Msg, typename El>
std::optional<std::span<const El>>
GetRepeatedFieldFromPtr(const Msg& msg, FieldPointer<Msg, El*> auto field_ptr) {
  const auto iter_opt = FindFieldByPointer<Msg, El*>(msg, field_ptr);
  if (iter_opt.has_value()) {
    const auto iter = *iter_opt;
    if (iter.pSize == nullptr) {
      return {};
    }

    const auto count = static_cast<std::size_t>(
        *reinterpret_cast<const pb_size_t*>(iter.pSize));

    return std::span{reinterpret_cast<const El*>(iter.pData), count};
  }

  return {};
}

template <typename PMsg, typename WMsg, FieldPointer<WMsg, uint32_t*> auto WF,
          FieldPointer<WMsg, PMsg> auto WB>
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

using MyParamMap = ParameterMapping<
    radiation_test_setup::TemperatureSensorCalibration,
    radiation_test_setup::WriteTemperatureSensorCalibrationRequest,
    &radiation_test_setup::WriteTemperatureSensorCalibrationRequest::fields,
    &radiation_test_setup::WriteTemperatureSensorCalibrationRequest::
        parameters>;

int main() {
  radiation_test_setup::TemperatureSensorCalibration             msg{};
  radiation_test_setup::WriteTemperatureSensorCalibrationRequest w_msg{};

  w_msg.fields[0]    = 1;
  w_msg.fields_count = 1;

  w_msg.parameters.sink_offset   = 20;
  w_msg.parameters.source_offset = 40;

  MyParamMap::ApplyWriteMessage(w_msg, msg);

  int a = 65;
}
