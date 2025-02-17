#pragma once

#include <hal/pin.h>

namespace hal::test::doubles {

class FakeGpi {
 public:
  constexpr explicit FakeGpi(bool value = false) noexcept
      : value{false} {}

  [[nodiscard]] constexpr bool Read() const noexcept { return value; }

  constexpr void SetValue(bool new_value) noexcept { value = new_value; }

 private:
  bool value;
};

static_assert(hal::Gpi<FakeGpi>);

class FakeGpo {
 public:
  constexpr explicit FakeGpo(bool value = false) noexcept
      : value{value} {}

  constexpr void Write(bool new_value) noexcept { value = new_value; }

  constexpr void Toggle() noexcept { value = !value; }

  [[nodiscard]] constexpr bool GetValue() const noexcept { return value; }

 private:
  bool value;
};

static_assert(hal::Gpo<FakeGpo>);

}   // namespace hal::test::doubles