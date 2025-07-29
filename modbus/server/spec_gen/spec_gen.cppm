module;

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

export module modbus.server.spec.gen;

import hstd;

import modbus.server.spec;

namespace modbus::server::spec::gen {

/**
 * Possible kinds of data supported by the MODBUS server
 */
enum DataKind {
  Scalar,   //!< Scalar type (integer, floating point)
  Enum,     //!< Enumerated type
  Array,    //!< Array of other types
};

/**
 * Given a type, returns its data kind
 * @tparam T Type to get the data kind of
 * @return Data kind
 */
template <typename T>
consteval DataKind GetDataKind() noexcept {
  if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
    return Scalar;
  } else if constexpr (std::is_enum_v<T>) {
    return Enum;
  } else if constexpr (hstd::concepts::Array<T>) {
    return Array;
  } else {
    std::unreachable();
  }
}

/**
 * Contains information on an enumerate member
 */
struct EnumMemberInfo {
  std::variant<int64_t, uint64_t> value;   //!< Associated value
  std::string                     name;    //!< Member name
};

/**
 * Contains information on an enum
 */
struct EnumInfo {
  std::string                 name;      //!< Enum name
  std::vector<EnumMemberInfo> members;   //!< Enum members
};

/**
 * Contains information on a register
 */
struct RegisterInfo {
  DataKind                  data_kind;   //!< Register data kind
  uint16_t                  address;     //!< Register starting address
  uint16_t                  size;        //!< Register size in registers
  std::string               name;        //!< Register name
  std::optional<EnumInfo>   enum_info;   //!< Register type enum info
  std::vector<RegisterInfo> children;    //!< Child registers
};

/**
 * Obtains the enum type info for an enum type register
 * @tparam Reg Register to obtain the enum info from
 * @return Enum info
 */
template <concepts::Register Reg>
EnumInfo GetEnumInfo() {
  static_assert(concepts::HasEnumDef<decltype(Reg::Options)>);

  using ED = std::decay_t<decltype(Reg::Options.enum_def)>;
  using UT = typename ED::UnderlyingType;

  using Members    = typename ED::Members;
  constexpr auto N = Members::Count;

  std::vector<EnumMemberInfo> members;
  ([&members]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
    (..., ([&members]<std::size_t Idx>(hstd::ValueMarker<Idx>) {
       using M = typename Members::template NthType<Idx>;

       if constexpr (std::is_signed_v<UT>) {
         members.push_back({
             .value = static_cast<int64_t>(M::Value),
             .name  = std::string{static_cast<std::string_view>(M::Name)},
         });
       } else {
         members.push_back({
             .value = static_cast<uint64_t>(M::Value),
             .name  = std::string{static_cast<std::string_view>(M::Name)},
         });
       }
     })(hstd::ValueMarker<Idxs>()));
  })(std::make_index_sequence<N>());

  return EnumInfo{
      .name    = std::string{static_cast<std::string_view>(ED::Name)},
      .members = members,
  };
}

/**
 * Obtains the register children for an array type register
 * @tparam Reg Register to obtain children for
 * @return Register children
 */
template <concepts::Register Reg>
std::vector<RegisterInfo> GetArrayRegisterChildren() {
  using Arr = std::decay_t<typename Reg::Data>;
  using DT  = typename Arr::value_type;

  std::vector<RegisterInfo> result{};
  result.reserve(hstd::ArraySize<Arr>);

  ([&result]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
    (..., ([&result]<std::size_t Idx>(hstd::ValueMarker<Idx>) {
       result.push_back(RegisterInfo{
           .data_kind = GetDataKind<DT>(),
           .address   = Reg::StartAddress + DataSize<DT>() * Idx,
           .size      = DataSize<DT>(),
           .name      = std::string{static_cast<std::string_view>(
               Reg::Options.array_element_naming
                   .template ElementName<Reg::Name, Idx>())},
       });
     })(hstd::ValueMarker<Idxs>{}));
  })(std::make_index_sequence<hstd::ArraySize<Arr>>{});

  return result;
}

/**
 * Obtains the RegisterInfo for a given register
 * @tparam Reg Register to determine info for
 * @return Register info
 */
export template <concepts::Register Reg>
RegisterInfo GetRegisterInfo() noexcept {
  constexpr auto DataKind = GetDataKind<typename Reg::Data>();
  RegisterInfo   result   = {
          .data_kind = GetDataKind<typename Reg::Data>(),
          .address   = Reg::StartAddress,
          .size      = Reg::Size,
          .name      = std::string{static_cast<std::string_view>(Reg::Name)},
          .enum_info = {},
          .children  = {},
  };

  // Handle specific data types
  if constexpr (DataKind == Array) {
    result.children = GetArrayRegisterChildren<Reg>();
  }

  if constexpr (DataKind == Enum) {
    result.enum_info = GetEnumInfo<Reg>();
  }

  return result;
}

}   // namespace modbus::server::spec::gen