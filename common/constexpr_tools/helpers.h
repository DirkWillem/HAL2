#pragma once

namespace ct {

template <auto V>
constexpr bool IsConstantExpression() {
  return true;
}

consteval bool Foo(auto a) {
  return true;
}

}   // namespace ct