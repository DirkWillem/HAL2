#pragma once

#include <string_view>
#include <utility>

namespace halstd {

/**
 * Performs a compile time assertion. When check is false, the behavior is
 * undefined
 * @param check Check to perform
 * @param message Diagnostic message
 */
consteval void Assert(bool                              check,
                      [[maybe_unused]] std::string_view message) noexcept {
  if (!check) {
    std::unreachable();
  }
}

}   // namespace halstd
