module;

#include <cstdint>
#include <limits>
#include <type_traits>

export module seq.hal:common;

namespace seq::hal {

export inline constexpr uint8_t PkgId = 2;

export enum class ModuleId : uint8_t { Uart = 1 };

export template <typename Id>
concept PeripheralId = std::is_enum_v<Id> || std::is_integral_v<Id>;

}   // namespace seq::hal
