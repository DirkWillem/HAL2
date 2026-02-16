module;

export module logging.test_helpers;

import logging.abstract;

namespace logging::test_helpers {

export template <concepts::Module M>
class ModuleLogger {
 public:
  using Module = M;

  template <concepts::Message Msg>
  void Trace(const Msg& message) {}

  template <concepts::Message Msg>
  void Debug(const Msg& message) {}

  template <concepts::Message Msg>
  void Info(const Msg& message) {}

  template <concepts::Message Msg>
  void Warn(const Msg& message) {}

  template <concepts::Message Msg>
  void Error(const Msg& message) {}

  template <concepts::Message Msg>
  void Fatal(const Msg& message) {}
};

}   // namespace logging::test_helpers