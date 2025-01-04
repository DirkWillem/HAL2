#pragma once

#include <concepts>
#include <cstdint>
#include <tuple>

#include <constexpr_tools/helpers.h>

#include <fp/concepts.h>

namespace sbs {

template <typename T>
/**
 * Describes a single signal
 * @tparam T Signal type
 */
concept SignalType =
    std::integral<T> || fp::FixedPointType<T> || std::is_same_v<T, float>;

template <SignalType T>
struct Signal {
  using Type = T;
};

template <typename S>
concept SignalDescriptor = requires {
  requires SignalType<typename S::Type>;

  { S::Name } -> std::convertible_to<std::string_view>;
  requires S::Name.length() <= 16;
};

template <typename T>
/**
 * Describes a frame ID type
 * @tparam T Frame ID type
 */
concept FrameId =
    std::is_enum_v<T>
    && (std::is_same_v<std::decay_t<std::underlying_type_t<T>>, uint32_t>);

template <auto I, SignalDescriptor... Sts>
  requires FrameId<std::decay_t<decltype(I)>>
/**
 * Base type for a frame
 * @tparam Id Frame type
 * @tparam Sts Signal types
 */
struct Frame {
  using FrameType    = Frame<I, Sts...>;
  using FrameIdType  = std::decay_t<decltype(I)>;
  using SignalsTuple = std::tuple<typename Sts::Type...>;

  static constexpr auto Id          = I;
  static constexpr auto NumSignals  = sizeof...(Sts);
  static constexpr auto PayloadSize = (0 + ... + sizeof(typename Sts::Type));
};

template <typename T>
struct FrameMeta : std::false_type {};

template <typename T>
struct is_frame : std::false_type {};

template <auto Id, typename... Sts>
struct is_frame<Frame<Id, Sts...>> : std::true_type {};

template <typename T>
inline constexpr bool is_frame_v = is_frame<T>::value;

template <typename F>
concept FrameType = requires {
  typename F::FrameType;
  requires is_frame_v<typename F::FrameType>;

  { F::Name } -> std::convertible_to<std::string_view>;
  requires F::Name.length() <= 16;
};

}   // namespace sbs

#define SIGNAL(StructName, Type, ReadableName)                    \
  struct StructName : sbs::Signal<Type> {                         \
    static constexpr auto Name = std::string_view{#ReadableName}; \
  };
