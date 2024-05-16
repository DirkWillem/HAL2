#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace ct {

/**
 * Parses a decimal unsigned integer string
 * @tparam T Integer type to parse into
 * @param string View of the string to parse
 * @return Parsed value, or std::nullopt if the string is not a valid
 * decimal integer
 */
template <std::unsigned_integral T>
constexpr std::optional<T>
ParseUnsignedDecimal(std::string_view string) noexcept {
  T result{0};

  for (auto c : string) {
    result <<= 4U;

    if (c >= '0' && c <= '9') {
      result += (c - '0');
    } else {
      return {};
    }
  }

  return result;
}

/**
 * Parses a hexadecimal unsigned integer string
 * @tparam T Integer type to parse into
 * @param string View of the string to parse
 * @return Parsed value, or std::nullopt if the string is not a valid
 * hexadecimal integer
 */
template <std::unsigned_integral T>
constexpr std::optional<T> ParseHexadecimal(std::string_view string) noexcept {
  constexpr std::size_t max_digits = sizeof(T) * 2;

  if (string.size() > max_digits) {
    return {};
  }

  T result{0};

  for (auto c : string) {
    result <<= 4U;

    if (c >= '0' && c <= '9') {
      result += (c - '0');
    } else if (c >= 'a' && c <= 'f') {
      result += (c - 'a') + 0xA;
    } else if (c >= 'A' && c <= 'F') {
      result += (c - 'A') + 0xA;
    } else {
      return {};
    }
  }

  return result;
}

}   // namespace ct
