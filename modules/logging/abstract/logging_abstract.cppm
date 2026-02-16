module;

#include <cstdint>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>

export module logging.abstract;

import hstd;

namespace logging {

/** @brief Log levels */
export enum class Level : uint8_t {
  Trace = 10,
  Debug = 20,
  Info  = 30,
  Warn  = 40,
  Error = 50,
  Fatal = 60,
};

export template <hstd::StaticString Msg, typename... Args>
class Message {
 public:
  explicit Message(Args... args)
      : args{std::forward<Args>(args)...} {}

  std::tuple<Args...> args;
};

export template <uint16_t I, hstd::StaticString N, typename... Msgs>
class Module {
 public:
  static constexpr uint16_t Id = I;

  template <typename Msg>
  static constexpr bool Contains() noexcept {
    return (... || std::is_same_v<Msg, Msgs>);
  }

  template <typename Msg>
    requires(Contains<Msg>())
  static constexpr std::size_t MessageIndex() noexcept {
    return *hstd::Types<Msgs...>::template IndexOf<Msg>();
  }
};

namespace concepts {

using PlainMessage  = Message<"">;
using ArgsMessage   = Message<"", uint32_t, float>;
using ExampleModule = Module<0, "", PlainMessage, ArgsMessage>;

export template <typename E>
concept Encoding = requires {
  {
    std::decay_t<E>::template Encode<ExampleModule, PlainMessage>(
        std::declval<uint32_t>(),              // Timestamp
        std::declval<Level>(),                 // Log level,
        std::declval<const PlainMessage&>(),   // Log message
        std::declval<std::span<std::byte>>()   // Destination buffer
    )
  };
  {
    std::decay_t<E>::template Encode<ExampleModule, ArgsMessage>(
        std::declval<uint32_t>(),              // Timestamp
        std::declval<Level>(),                 // Log level,
        std::declval<const ArgsMessage&>(),    // Log message
        std::declval<std::span<std::byte>>()   // Destination buffer
    )
  };
};

export template <typename S>
concept Sink = requires(S& sink) {
  { sink.Write(std::span<const std::byte>()) };
};

template <typename T>
inline constexpr bool IsMessage = false;

template <auto Msg, typename... Args>
inline constexpr bool IsMessage<Message<Msg, Args...>> = true;

/**
 * @brief Concept for a log message.
 */
export template <typename T>
concept Message = IsMessage<std::decay_t<T>>;

template <typename T>
inline constexpr bool IsModule = false;

template <uint16_t I, auto N, typename... Msgs>
inline constexpr bool IsModule<Module<I, N, Msgs...>> = true;

/**
 * @brief Concept for a log message.
 */
export template <typename T>
concept Module = IsModule<std::decay_t<T>>;

export template <typename L, typename M>
concept ModuleLogger = requires(L& ml) {
  typename L::Module;

  requires std::is_same_v<std::decay_t<typename L::Module>, M>;
};

}   // namespace concepts

}   // namespace logging