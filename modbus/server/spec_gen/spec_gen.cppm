module;

#include <algorithm>
#include <map>
#include <ranges>
#include <vector>

#include <nlohmann/json.hpp>

export module modbus.server.spec.gen;

export import :registers;

namespace modbus::server::spec::gen {

using namespace nlohmann;

/**
 * Returns the register info for a given register
 * @tparam Reg Register to get info for
 * @return Register info for the register type
 */
export template <typename Reg>
RegisterInfo GetRegisterInfo() {
  return RegisterInfo{
      .name    = Reg::Name,
      .address = Reg::Address,
  };
}
export template <hstd::concepts::Types Regs>
std::vector<RegisterInfo> GetRegistersInfo() {
  std::vector<RegisterInfo> result{};
  result.reserve(Regs::Count);

  Regs::ForEach([&result]<typename Reg>(hstd::Marker<Reg>) {
    result.emplace_back(GetRegisterInfo<Reg>());
  });

  return result;
}

/**
 * Returns the JSON name for a given ScalarType
 * @param st Scalar type to get JSON name for
 * @return JSON name for the scalar type
 */
export std::string GetScalarTypeJsonName(ScalarType st) {
  switch (st) {
  case ScalarType::U8: return "uint8";
  case ScalarType::U16: return "uint16";
  case ScalarType::U32: return "uint32";
  case ScalarType::U64: return "uint64";
  case ScalarType::I8: return "int8";
  case ScalarType::I16: return "int16";
  case ScalarType::I32: return "int32";
  case ScalarType::I64: return "int64";
  case ScalarType::F32: return "float32";
  case ScalarType::F64: return "float64";
  default: std::unreachable();
  }
}

/**
 * Returns a JSON representation of a DataType
 * @param type Data type to get JSON for
 * @return JSON representation of the data type
 */
export json GetDataTypeJson(const DataType& type) {
  if (const auto scalar = std::get_if<ScalarType>(&type.type); scalar) {
    return {{"type", GetScalarTypeJsonName(*scalar)}};
  }

  if (const auto enum_ref = std::get_if<EnumRef>(&type.type); enum_ref) {
    return {
        {"type", "enum"},
        {"enum_name", enum_ref->name},
    };
  }

  if (const auto array_type = std::get_if<ArrayType>(&type.type); array_type) {
    return {
        {"type", "array"},
        {"size", array_type->size},
        {"element_type", GetDataTypeJson(*array_type->element_type)},
    };
  }

  std::unreachable();
}

/**
 * Returns a JSON representation of an EnumInfo
 * @param info Enum info to get JSON for
 * @return JSON representation of the enum info
 */
export json GetEnumSpecJson(const EnumInfo& info) {
  const auto members =
      info.members
      | std::views::transform([](const EnumMemberInfo& member) -> json {
          if (const auto i64 = std::get_if<int64_t>(&member.value); i64) {
            return {{"name", member.name}, {"value", *i64}};
          }

          if (const auto u64 = std::get_if<uint64_t>(&member.value); u64) {
            return {{"name", member.name}, {"value", *u64}};
          }

          std::unreachable();
        })
      | std::ranges::to<std::vector<json>>();

  return {
      {"name", info.name},
      {"underlying_type", GetScalarTypeJsonName(info.underlying_type)},
      {"members", members},
  };
}

/**
 * Returns a JSON representation of all enums in the given registers
 * @tparam Regs Registers to get enums for
 * @param regs Registers to get enums for
 * @return JSON representation of all enums in the given registers
 */
export template <std::ranges::view... Regs>
json GetEnumSpecsJson(Regs... regs) {
  std::map<std::string, json> enums{};

  (..., std::ranges::for_each(regs, [&enums](const RegisterInfo& reg_info) {
     if (reg_info.enum_info.has_value()) {
       const auto& enum_info = *reg_info.enum_info;

       if (!enums.contains(enum_info.name)) {
         enums[enum_info.name] = GetEnumSpecJson(enum_info);
       };
     }
   }));

  return enums;
}

/**
 * Returns a JSON representation of a RegisterInfo
 * @param info Register info to get JSON for
 * @return JSON representation of the register info
 */
export json GetRegisterInfoJson(const RegisterInfo& info) {
  json result{
      {"name", info.name},
      {"start_address", info.address},
      {"size", info.size},
      {"type", GetDataTypeJson(*info.data_type)},
  };

  if (!info.children.empty()) {
    result["children"] = info.children
                         | std::views::transform([](const RegisterInfo& info) {
                             return GetRegisterInfoJson(info);
                           })
                         | std::ranges::to<std::vector<json>>();
  }

  return result;
}

export template <concepts::ServerSpec Spec>
json GetServerSpecJson() {
  auto input_registers   = GetRegistersInfo<typename Spec::InputRegisters>();
  auto holding_registers = GetRegistersInfo<typename Spec::HoldingRegisters>();

  std::vector<json> holding_registers_json{};
  holding_registers_json.reserve(holding_registers.size());

  std::ranges::transform(
      holding_registers, std::back_inserter(holding_registers_json),
      [](const auto& info) { return GetRegisterInfoJson(info); });

  return {
      {"enums", GetEnumSpecsJson(std::views::all(input_registers),
                                 std::views::all(holding_registers))},
      {"holding_registers", holding_registers_json},
  };
}

}   // namespace modbus::server::spec::gen