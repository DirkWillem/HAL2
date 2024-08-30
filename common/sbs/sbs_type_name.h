#pragma once

#include <cstdint>
#include <string_view>

#include <fp/fix.h>

namespace sbs {

template <typename T>
struct TypeNameHelper;

#define TYPE_NAME(Type, Name)                             \
  template <>                                             \
  struct TypeNameHelper<Type> {                           \
    static constexpr auto Name = std::string_view{#Name}; \
  };

TYPE_NAME(uint8_t, uint8)
TYPE_NAME(uint16_t, uint16)
TYPE_NAME(uint32_t, uint32)

TYPE_NAME(int8_t, int8)
TYPE_NAME(int16_t, int16)
TYPE_NAME(int32_t, int32)

TYPE_NAME(float, float32)

template <fp::FixedPointType FP>
struct TypeNameHelper<FP> {
  static constexpr auto Name = FP::Name.view();
};

template <typename T>
inline constexpr std::string_view TypeName = TypeNameHelper<T>::Name;

}   // namespace sbs