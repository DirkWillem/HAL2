module;

#include <array>

export module logging;

import hstd;

export import logging.abstract;

export import :encoding.binary;

export import :sink.usb_cdc;

namespace logging {

template <hstd::Clock C, concepts::Encoding E, concepts::Sink S,
          concepts::Module M>
class ModuleLogger {
 public:
  /** @brief Module */
  using Module = M;

  explicit ModuleLogger(S& sink)
      : sink{sink} {}

  /**
   * @brief Logs a message at the \c Trace level.
   * @tparam Msg Message type.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Trace(const Msg& message) {
    Log(Level::Trace, message);
  }

  /**
   * @brief Logs a message at the \c Debug level.
   * @tparam Msg Message type.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Debug(const Msg& message) {
    Log(Level::Debug, message);
  }

  /**
   * @brief Logs a message at the \c Info level.
   * @tparam Msg Message type.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Info(const Msg& message) {
    Log(Level::Info, message);
  }

  /**
   * @brief Logs a message at the \c Warn level.
   * @tparam Msg Message type.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Warn(const Msg& message) {
    Log(Level::Warn, message);
  }

  /**
   * @brief Logs a message at the \c Error level.
   * @tparam Msg Message type.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Error(const Msg& message) {
    Log(Level::Error, message);
  }

  /**
   * @brief Logs a message at the \c Fatal level.
   * @tparam Msg Message type.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Fatal(const Msg& message) {
    Log(Level::Fatal, message);
  }

  /**
   * @brief Logs a message.
   * @tparam Msg Message to log.
   * @param level Log level to use.
   * @param message Message to log.
   */
  template <concepts::Message Msg>
    requires(M::template Contains<Msg>())
  void Log(Level level, const Msg& message) {
    std::array<std::byte, 64> buffer{};
    const auto                ts = C::now().time_since_epoch().count();
    const auto encoded = E::template Encode<M, Msg>(ts, level, message, buffer);

    sink.Write(encoded);
  }

 private:
  S& sink;
};

/**
 * @brief
 * @tparam C Clock type.
 * @tparam E Encoding to use.
 * @tparam S Sink to use.
 * @tparam Modules Modules to register in the logger
 */
export template <hstd::Clock C, concepts::Encoding E, concepts::Sink S,
                 concepts::Module... Modules>
class Logger {
 public:
  template <concepts::Module M>
    requires(... || std::is_same_v<M, Modules>)
  using Module = ModuleLogger<C, E, S, M>;

  explicit Logger(S& sink)
      : sink{sink} {}

  template <concepts::Module M>
    requires(... || std::is_same_v<M, Modules>)
  [[nodiscard]] auto GetModule() const noexcept {
    return ModuleLogger<C, E, S, M>{sink};
  }

 private:
  S& sink;
};

}   // namespace logging