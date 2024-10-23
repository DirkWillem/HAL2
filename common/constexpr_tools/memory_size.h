#pragma once

#include <cstdlib>
#include <limits>
#include <utility>

namespace ct {

inline constexpr std::size_t Byte     = 1;
inline constexpr std::size_t Kibibyte = 1024 * Byte;
inline constexpr std::size_t Mebibyte = 1024 * Kibibyte;
inline constexpr std::size_t Gibibyte = 1024 * Mebibyte;

namespace literals {

constexpr std::size_t operator""_B(unsigned long long int v) {
  if (v >= static_cast<unsigned long long int>(
          std::numeric_limits<std::size_t>::max())) {
    std::unreachable();
  }

  return Byte * v;
}

constexpr std::size_t operator""_KiB(unsigned long long int v) {
  if (v * Kibibyte > static_cast<unsigned long long int>(
          std::numeric_limits<std::size_t>::max())) {
    std::unreachable();
  }

  return Kibibyte * static_cast<std::size_t>(v);
}

constexpr std::size_t operator""_MiB(unsigned long long int v) {
  if (v * Mebibyte > static_cast<unsigned long long int>(
          std::numeric_limits<std::size_t>::max())) {
    std::unreachable();
  }

  return Mebibyte * static_cast<std::size_t>(v);
}

constexpr std::size_t operator""_GiB(unsigned long long int v) {
  if (v * Gibibyte > static_cast<unsigned long long int>(
          std::numeric_limits<std::size_t>::max())) {
    std::unreachable();
  }

  return Gibibyte * static_cast<std::size_t>(v);
}

}   // namespace literals
}   // namespace ct