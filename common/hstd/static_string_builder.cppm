module;

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

export module hstd:static_string_builder;

import :math;

namespace hstd {

export template <std::size_t N>
class StaticStringBuilder {
 public:
  template <std::integral T>
  constexpr bool Append(T value) noexcept {
    // Append sign if applicable
    if constexpr (std::is_signed_v<T>) {
      if (value < 0) {
        if (!AppendChar('-')) {
          return false;
        }

        value = -value;
      }
    }

    return WriteUnsignedIntDecimal(static_cast<unsigned>(value));
  }

  constexpr bool Append(std::string_view value) noexcept {
    if (offset + value.length() >= buffer.size()) {
      if consteval {
        std::unreachable();
      }
      return false;
    }

    std::copy(value.begin(), value.end(), &buffer[offset]);
    offset += value.length();
    return true;
  }

  constexpr bool AppendChar(char v) noexcept {
    if (offset >= buffer.size()) {
      if consteval {
        std::unreachable();
      }
      return false;
    }

    buffer[offset] = v;
    offset++;
    return true;
  }

  [[nodiscard]] constexpr auto view() const noexcept {
    return operator std::string_view();
  }

  explicit constexpr operator std::string_view() const noexcept {
    return std::string_view{buffer.data(), offset};
  }

 private:
  constexpr bool WriteUnsignedIntDecimal(unsigned value) noexcept {
    if (value == 0) {
      return AppendChar('0');
    }

    std::array<char,
               NumDigits<unsigned>(std::numeric_limits<unsigned>::max(), 10)>
        digit_values{};

    std::size_t n_digits = 0;
    while (value > 0) {
      const auto cur_digit   = value % 10;
      digit_values[n_digits] = static_cast<char>('0' + cur_digit);
      n_digits++;
      value /= 10;
    }

    for (std::size_t i = 0; i < n_digits; i++) {
      if (!AppendChar(digit_values[n_digits - i - 1])) {
        return false;
      }
    }

    return true;
  }

  std::size_t         offset{0};
  std::array<char, N> buffer{};
};

}   // namespace hstd