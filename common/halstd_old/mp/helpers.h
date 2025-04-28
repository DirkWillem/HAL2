#pragma once


namespace halstd {

template <auto V>
constexpr bool IsConstantExpression() {
  return true;
}

}