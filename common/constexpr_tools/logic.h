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

}   // namespace ct