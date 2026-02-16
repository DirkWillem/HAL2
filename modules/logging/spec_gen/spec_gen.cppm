module;

#include <exception>
#include <format>
#include <fstream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <argparse/argparse.hpp>

import logging.abstract;

import hstd;

export module logging.spec_gen;

namespace logging::spec_gen {

using namespace nlohmann;

/** @brief Descriptor of a log message argument. */
struct ArgumentDescriptor {
  std::string                name;     //!< Argument name.
  std::string                type;     //!< Argument type name.
  std::optional<std::string> format;   //!< Format specifier.

  [[nodiscard]] json ToJson() const {
    return {
        {"name", name},
        {"type", type},
        {"format", format.has_value() ? json{*format} : nullptr},
    };
  }
};

/** @brief Descriptor of a log message. */
struct MessageDescriptor {
  uint8_t     id;                              //!< Message ID.
  std::string message_template;                //!< Message template string.
  std::vector<ArgumentDescriptor> arguments;   //!< Message arguments.

  [[nodiscard]] json ToJson() const {
    json result = {
        {"id", id},
        {"message_template", message_template},
    };

    result["arguments"] =
        arguments
        | std::views::transform([](const auto& a) { return a.ToJson(); })
        | std::ranges::to<std::vector<json>>();

    return result;
  }
};

/** @brief Descriptor of a log module. */
struct ModuleDescriptor {
  uint16_t    id;     //!< Module ID.
  std::string name;   //!< Module name.
  std::vector<MessageDescriptor>
      messages;   //!< Messages that can be produced by the module.

  [[nodiscard]] json ToJson() const {
    json result = {
        {"id", id},
        {"name", name},
    };

    result["messages"] =
        messages
        | std::views::transform([](const auto& a) { return a.ToJson(); })
        | std::ranges::to<std::vector<json>>();

    return result;
  }
};

template <typename T>
std::string GetArgType() {
  if constexpr (std::is_same_v<T, uint8_t>) {
    return "uint8";
  } else if constexpr (std::is_same_v<T, uint16_t>) {
    return "uint16";
  } else if constexpr (std::is_same_v<T, uint32_t>) {
    return "uint32";
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    return "uint64";
  } else if constexpr (std::is_same_v<T, int8_t>) {
    return "int8";
  } else if constexpr (std::is_same_v<T, int16_t>) {
    return "int16";
  } else if constexpr (std::is_same_v<T, int32_t>) {
    return "int32";
  } else if constexpr (std::is_same_v<T, int64_t>) {
    return "int64";
  } else if constexpr (std::is_same_v<T, float>) {
    return "float32";
  } else if constexpr (std::is_same_v<T, double>) {
    return "float64";
  } else if constexpr (std::is_enum_v<T>) {
    return GetArgType<std::underlying_type_t<T>>();
  } else {
    return "!unknown";
  }
}

enum class MessageParseState {
  MessageBody,
  OpeningBrace,
  ArgumentName,
  ArgumentFormat,
  Illegal,
};

template <typename T>
struct MessageHelper;

template <hstd::StaticString MsgTemplate, typename... Args>
struct MessageHelper<Message<MsgTemplate, Args...>> {
  static MessageDescriptor GetDescriptor(uint8_t id) {
    const std::string msg_template{static_cast<std::string_view>(MsgTemplate)};

    std::array<std::string, sizeof...(Args)> arg_types = {
        GetArgType<Args>()...};
    auto parsed_args = ParseArguments(msg_template);

    if (arg_types.size() != parsed_args.size()) {
      throw std::runtime_error{std::format(
          "Mismatch in number of arguments provided in message type ({}) and "
          "number of arguments parsed from message '{}' ({})",
          arg_types.size(), msg_template, parsed_args.size())};
    }

    std::vector<ArgumentDescriptor> args{};
    args.reserve(arg_types.size());

    for (std::size_t i = 0; i < arg_types.size(); ++i) {
      args.push_back({
          .name   = parsed_args[i].first,
          .type   = arg_types[i],
          .format = parsed_args[i].second,
      });
    }

    return {
        .id               = id,
        .message_template = msg_template,
        .arguments        = args,
    };
  }

 private:
  static std::vector<std::pair<std::string, std::optional<std::string>>>
  ParseArguments(std::string_view msg) {
    using enum MessageParseState;

    std::vector<std::pair<std::string, std::optional<std::string>>> result{};
    auto state = MessageBody;

    std::stringstream name_ss{};
    std::stringstream format_ss{};

    for (char c : msg) {
      switch (state) {
      case MessageBody:
        if (c == '{') {
          state = OpeningBrace;
        }
        break;
      case OpeningBrace:
        if (c == '{') {
          state = MessageBody;
        } else if (std::isalpha(c) || c == '_') {
          name_ss.str("");
          name_ss << c;
          state = ArgumentName;
        } else {
          state = Illegal;
          break;
        }
        break;
      case ArgumentName:
        if (std::isalnum(c) || c == '_') {
          name_ss << c;
        } else if (c == '}') {
          result.push_back({name_ss.str(), std::nullopt});
          state = MessageBody;
        } else if (c == ':') {
          format_ss.clear();
          state = ArgumentFormat;
        }
        break;
      case ArgumentFormat:
        if (std::isdigit(c) || c == '.' || c == '<' || c == '>') {
          format_ss << c;
        } else if (c == '}') {
          result.push_back({name_ss.str(), format_ss.str()});
          state = MessageBody;
        } else {
          state = Illegal;
        }
        break;
      case Illegal: break;
      }
    }

    if (state == Illegal) {
      throw std::runtime_error{"Invalid message template string"};
    }

    return result;
  }
};

template <typename T>
struct ModuleHelper;

template <uint16_t Id, hstd::StaticString Name, typename... Msgs>
struct ModuleHelper<Module<Id, Name, Msgs...>> {
  static ModuleDescriptor GetDescriptor() {
    using Mod = Module<Id, Name, Msgs...>;

    return {
        .id       = Id,
        .name     = std::string{static_cast<std::string_view>(Name)},
        .messages = {MessageHelper<Msgs>::GetDescriptor(
            Mod::template MessageIndex<Msgs>() + 1)...},
    };
  }
};

export template <concepts::Module... Modules>
int GenerateLoggingSpec(std::string app_name, int argc, const char** argv) {
  argparse::ArgumentParser parser{std::move(app_name)};

  parser.add_argument("-o", "--output")
      .help("Output file")
      .required()
      .help("File to which to write the MODBUS server specification");

  try {
    parser.parse_args(argc, argv);

    std::vector<json> modules_json{
        ModuleHelper<Modules>::GetDescriptor().ToJson()...};

    json result_json = {
      {"modules", modules_json},
    };

    std::filesystem::path output_path{parser.get<std::string>("output")};
    std::ofstream         output{output_path};
    output << std::setw(2) << result_json << std::endl;

    std::cout << "Wrote Logging Specification to " << output_path << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}

}   // namespace logging::spec_gen