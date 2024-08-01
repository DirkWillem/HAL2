#pragma once

#include <bit>
#include <cstring>
#include <memory>
#include <span>

namespace ct {

class BufferReader {
 public:
  explicit constexpr BufferReader(std::span<const std::byte> data) noexcept
      : data{data} {}

  template <typename T>
    requires(!std::is_same_v<T, std::byte>)
  constexpr bool Read(T& into) noexcept {
    if (!ok || data.size() < sizeof(T)) {
      ok = false;
      return false;
    }

    std::array<std::byte, sizeof(T)> tmp_data{};
    std::memcpy(tmp_data.data(), data.data(), sizeof(T));
    into = std::bit_cast<T>(tmp_data);
    data = data.subspan(sizeof(T));

    return true;
  }

  template <typename T>
    requires(std::is_same_v<T, std::byte>)
  constexpr bool Read(T& into) noexcept {
    if (!ok || data.empty()) {
      ok = false;
      return false;
    }

    into = data[0];
    data = data.subspan(1);
    return true;
  }

  template <std::equality_comparable T>
  constexpr bool ReadLiteral(T value) noexcept {
    T check{};
    if (!Read(check)) {
      return false;
    }

    if (value != check) {
      ok = false;
    }
    return ok;
  }

  [[nodiscard]] constexpr bool valid() const noexcept { return ok; }

 private:
  bool                       ok{true};
  std::span<const std::byte> data;
};

class BufferWriter {
 public:
  explicit constexpr BufferWriter(std::span<std::byte> data) noexcept
      : original_data{data}
      , data{data} {}

  template <typename T>
  constexpr bool Write(T value) noexcept {
    if (!ok || sizeof(value) > data.size()) {
      ok = false;
      return false;
    }

    std::memcpy(data.data(), &value, sizeof(T));
    n_written += sizeof(T);
    data = data.subspan(sizeof(T));

    return true;
  }

  constexpr bool WriteString(std::string_view value) noexcept {
    if (!ok || value.length() > data.size()) {
      ok = false;
      return false;
    }

    std::memcpy(data.data(), value.data(), value.length());
    n_written += value.length();
    data = data.subspan(value.length());

    return true;
  }

  [[nodiscard]] constexpr bool valid() const noexcept { return ok; }

  [[nodiscard]] constexpr std::span<std::byte> WrittenData() const noexcept {
    return original_data.subspan(0, n_written);
  }

 private:
  bool                 ok{true};
  std::size_t          n_written{0};
  std::span<std::byte> original_data;
  std::span<std::byte> data;
};

}   // namespace ct