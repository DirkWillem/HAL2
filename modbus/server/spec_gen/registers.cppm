module;

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

export module modbus.server.spec.gen:registers;

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
 * Given an array type, returns the data kind of its element type
 * @tparam T Array type to get the element data kind of
 * @return Array element data kind
 */
template <hstd::concepts::Array T>
consteval DataKind GetArrayElementDataKind() noexcept {
  using ET = typename T::value_type;
  return GetDataKind<ET>();
}

export struct DataType;

/**
 * Enum containing the possible scalar types
 */
export enum class ScalarType {
  U8,    //!< Unsigned 8-bit integer
  U16,   //!< Unsigned 16-bit integer
  U32,   //!< Unsigned 32-bit integer
  U64,   //!< Unsigned 64-bit integer
  I8,    //!< Signed 8-bit integer
  I16,   //!< Signed 16-bit integer
  I32,   //!< Signed 32-bit integer
  I64,   //!< Signed 64-bit integer
  F32,   //!< 32-bit floating point number
  F64,   //!< 64-bit floating point number
};

template <typename T>
constexpr ScalarType GetScalarType() noexcept {
  if constexpr (std::is_same_v<T, uint8_t>) {
    return ScalarType::U8;
  } else if constexpr (std::is_same_v<T, uint16_t>) {
    return ScalarType::U16;
  } else if constexpr (std::is_same_v<T, uint32_t>) {
    return ScalarType::U32;
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    return ScalarType::U64;
  } else if constexpr (std::is_same_v<T, int8_t>) {
    return ScalarType::I8;
  } else if constexpr (std::is_same_v<T, int16_t>) {
    return ScalarType::I16;
  } else if constexpr (std::is_same_v<T, int32_t>) {
    return ScalarType::I32;
  } else if constexpr (std::is_same_v<T, int64_t>) {
    return ScalarType::I64;
  } else if constexpr (std::is_same_v<T, float>) {
    return ScalarType::F32;
  } else if constexpr (std::is_same_v<T, double>) {
    return ScalarType::F64;
  }

  std::unreachable();
}

/**
 * Reference to an enum type
 */
export struct EnumRef {
  std::string name;   //!< Enum type name
};

/**
 * Array type
 */
export struct ArrayType {
  std::size_t               size;           //!< Array size
  std::unique_ptr<DataType> element_type;   //!< Array element type
};

/**
 * Represents the data type of a register
 */
export struct DataType {
  std::variant<ScalarType, EnumRef, ArrayType> type;   //!< Actual type

  /**
   * Returns the kind of the data type
   * @return Data type kind
   */
  [[nodiscard]] constexpr DataKind GetKind() const noexcept {
    if (std::holds_alternative<ScalarType>(type)) {
      return Scalar;
    } else if (std::holds_alternative<EnumRef>(type)) {
      return Enum;
    } else if (std::holds_alternative<ArrayType>(type)) {
      return Array;
    }

    std::unreachable();
  }
};

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
  std::string                 name;              //!< Enum name
  ScalarType                  underlying_type;   //!< Underlying type
  std::vector<EnumMemberInfo> members;           //!< Enum members
};

/**
 * Contains information on a register
 */
struct RegisterInfo {
  DataKind                  data_kind;   //!< Register data kind
  std::unique_ptr<DataType> data_type;   //!< Register data type
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
template <concepts::EnumDef auto EDv>
EnumInfo GetEnumInfo() {
  using ED = std::decay_t<decltype(EDv)>;
  using UT = std::decay_t<typename ED::UnderlyingType>;

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
      .name            = std::string{static_cast<std::string_view>(ED::Name)},
      .underlying_type = GetScalarType<UT>(),
      .members         = members,
  };
}

/**
 * Obtains the data type for a given register
 * @tparam Reg Register to obtain the data type for
 * @tparam T Type to get the data type of
 * @return Data type
 */
template <concepts::Register Reg, typename T = typename Reg::Data>
std::unique_ptr<DataType> GetDataType() noexcept {
  constexpr auto Kind = GetDataKind<T>();

  if constexpr (Kind == Scalar) {
    return std::make_unique<DataType>(GetScalarType<T>());
  } else if constexpr (Kind == Enum) {
    const auto enum_info = GetEnumInfo<Reg::Options.enum_def>();
    return std::make_unique<DataType>(EnumRef{.name = enum_info.name});
  } else if constexpr (Kind == Array) {
    using ET            = typename T::value_type;
    constexpr auto Size = hstd::ArraySize<T>;

    return std::make_unique<DataType>(ArrayType{
        .size         = Size,
        .element_type = GetDataType<Reg, ET>(),
    });
  }

  std::unreachable();
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
           .data_type = GetDataType<Reg>(),
           .address   = Reg::StartAddress + DataSize<DT>() * Idx,
           .size      = DataSize<DT>(),
           .name      = std::string{static_cast<std::string_view>(
               Reg::Options.array_element_naming
                   .template ElementName<Reg::Name, Idx>())},
           .enum_info = {},
           .children  = {},
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
  constexpr auto DataKind  = GetDataKind<typename Reg::Data>();
  auto           data_type = GetDataType<Reg>();

  // Handle specific data types
  if constexpr (DataKind == Scalar) {
    return {
        .data_kind = DataKind,
        .data_type = std::move(data_type),
        .address   = Reg::StartAddress,
        .size      = Reg::Size,
        .name      = std::string{static_cast<std::string_view>(Reg::Name)},
        .enum_info = {},
        .children  = GetArrayRegisterChildren<Reg>(),
    };
  }

  if constexpr (DataKind == Array) {
    RegisterInfo result = {
        .data_kind = DataKind,
        .data_type = std::move(data_type),
        .address   = Reg::StartAddress,
        .size      = Reg::Size,
        .name      = std::string{static_cast<std::string_view>(Reg::Name)},
        .enum_info = {},
        .children  = GetArrayRegisterChildren<Reg>(),
    };

    constexpr auto ElDataKind = GetArrayElementDataKind<typename Reg::Data>();
    if constexpr (ElDataKind == Enum) {
      static_assert(concepts::HasEnumDef<std::decay_t<decltype(Reg::Options)>>);
      result.enum_info = GetEnumInfo<Reg::Options.enum_def>();
    }

    return result;
  }

  if constexpr (DataKind == Enum) {
    static_assert(concepts::HasEnumDef<std::decay_t<decltype(Reg::Options)>>);
    const auto enum_info = GetEnumInfo<Reg::Options.enum_def>();
    return {
        .data_kind = DataKind,
        .data_type = std::move(data_type),
        .address   = Reg::StartAddress,
        .size      = Reg::Size,
        .name      = std::string{static_cast<std::string_view>(Reg::Name)},
        .enum_info = enum_info,
        .children  = {},
    };
  }

  std::unreachable();
}

}   // namespace modbus::server::spec::gen
