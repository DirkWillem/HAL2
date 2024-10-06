#pragma once

namespace ct {

/**
 * Logical implication operator, i.e. a => b
 * @param a Left-hand side of expression
 * @param b Right-hand side of expression
 * @return a => b
 */
[[nodiscard]] constexpr bool Implies(bool a, bool b) noexcept {
  return !a || b;
}

/**
 * Logical XOR operator, i.e. a xor b
 * @param a Left-hand side of expression
* @param b Right-hand side of expression
* @return a xor b
 */
[[nodiscard]] constexpr bool Xor(bool a, bool b) noexcept {
  return a != b;
}

template<typename T, auto... Values>
concept ComparableToAll = (... && std::equality_comparable_with<decltype(Values), T>);

template<auto... Values>
[[nodiscard]] constexpr bool IsOneOf(ComparableToAll<Values...> auto value) noexcept {
  return (... || (value == Values));
}

}   // namespace ct