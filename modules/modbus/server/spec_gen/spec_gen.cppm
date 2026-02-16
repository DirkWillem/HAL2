module;

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <argparse/argparse.hpp>

export module modbus.server.spec.gen;

export import :bits;
export import :registers;

namespace modbus::server::spec::gen {

using namespace nlohmann;

/**
 * Returns the bits info for all bits in a given type
 * @tparam Bits Bits to get info for
 * @return Bits info for all registers in the given type
 */
export template <hstd::concepts::Types Bits>
std::vector<BitsInfo> GetBitsInfo() {
  std::vector<BitsInfo> result{};
  result.reserve(Bits::Count);

  Bits::ForEach([&result]<typename Bit>(hstd::Marker<Bit>) {
    result.emplace_back(GetBitsInfo<Bit>());
  });

  return result;
}

/**
 * Returns the JSON string for a given BitAccess
 * @param access Bit access to get JSON string for
 * @return JSON string for the bit access
 */
export std::string GetBitAccessJsonString(BitAccess access) {
  switch (access) {
  case BitAccess::ReadWrite: return "rw";
  case BitAccess::ReadWrite0: return "rw0";
  case BitAccess::ReadWrite1: return "rw1";
  case BitAccess::DiscreteInput: return "";
  default: std::unreachable();
  }
}

/**
 * Returns a JSON representation of a BitInfo
 * @param info Bit info to get JSON for
 * @return JSON representation of the bit info
 */
export json GetBitInfoJson(const BitInfo& info) {
  json result = {
      {"name", info.name},
      {"address", info.address},
  };

  if (info.access != BitAccess::DiscreteInput) {
    result["access"] = GetBitAccessJsonString(info.access);
  }

  return result;
}

/**
 * Returns a JSON representation of a BitsInfo
 * @param info Bits info to get JSON for
 * @return JSON representation of the bits info
 */
export json GetBitsInfoJson(const BitsInfo& info) {
  json result = {
      {"name", info.name},
      {"start_address", info.address},
      {"size", info.size},
  };

  if (!info.bits.empty()) {
    result["bits"] = info.bits | std::views::transform(GetBitInfoJson)
                     | std::ranges::to<std::vector<json>>();
  }

  if (info.access != BitAccess::DiscreteInput) {
    result["access"] = GetBitAccessJsonString(info.access);
  }

  return result;
}

/**
 * Returns the register info for all registers in a given type
 * @tparam Regs Registers to get info for
 * @return Register info for all registers in the given type
 */
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
                         | std::views::transform(GetRegisterInfoJson)
                         | std::ranges::to<std::vector<json>>();
  }

  return result;
}

export template <concepts::ServerSpec Spec>
json GetServerSpecJson() {
  auto discrete_inputs   = GetBitsInfo<typename Spec::DiscreteInputs>();
  auto coils             = GetBitsInfo<typename Spec::Coils>();
  auto input_registers   = GetRegistersInfo<typename Spec::InputRegisters>();
  auto holding_registers = GetRegistersInfo<typename Spec::HoldingRegisters>();

  return {
      {
          "types",
          {{
              "enums",
              GetEnumSpecsJson(std::views::all(input_registers),
                               std::views::all(holding_registers)),
          }},
      },
      {"discrete_inputs", discrete_inputs
                              | std::views::transform(GetBitsInfoJson)
                              | std::ranges::to<std::vector<json>>()},

      {"coils", coils | std::views::transform(GetBitsInfoJson)
                    | std::ranges::to<std::vector<json>>()},
      {"input_registers", input_registers
                              | std::views::transform(GetRegisterInfoJson)
                              | std::ranges::to<std::vector<json>>()},
      {"holding_registers", holding_registers
                                | std::views::transform(GetRegisterInfoJson)
                                | std::ranges::to<std::vector<json>>()},

  };
}

/**
 * Implementation for generating a server spec from the command line
 * @tparam Spec Specification type
 * @param app_name Name of the application the function is invoked from
 * @param argc argument count
 * @param argv Pointer to arguments
 * @return Exit code
 */
export template <concepts::ServerSpec Spec>
int GenerateServerSpecCli(std::string app_name, int argc, const char** argv) {
  argparse::ArgumentParser parser{std::move(app_name)};

  parser.add_argument("-o", "--output")
      .help("Output file")
      .required()
      .help("File to which to write the MODBUS server specification");

  try {
    parser.parse_args(argc, argv);

    const auto server_json = GetServerSpecJson<Spec>();

    std::filesystem::path output_path{parser.get<std::string>("output")};
    std::ofstream         output{output_path};
    output << std::setw(2) << server_json << std::endl;

    std::cout << "Wrote MODBUS Server Specification to " << output_path
              << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}

}   // namespace modbus::server::spec::gen