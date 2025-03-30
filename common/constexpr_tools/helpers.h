#pragma once

namespace ct {

template <auto V>
constexpr bool IsConstantExpression() {
  return true;
}

}   // namespace ct