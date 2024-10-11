#pragma once

#include <array>

#include "std_ex.h"

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

template <typename T, auto... Values>
concept ComparableToAll =
    (... && std::equality_comparable_with<decltype(Values), T>);

template <auto... Values>
[[nodiscard]] constexpr bool
IsOneOf(ComparableToAll<Values...> auto value) noexcept {
  return (... || (value == Values));
}
//
//namespace detail {
//
//template <typename T, template <T V> typename Predicate, T... Values>
//  requires(... && stdex::BoolConstant<Predicate<Values>>)
//consteval std::size_t CountPredicateMatches() {
//  return (... + (Predicate<Values>::value ? 1 : 0));
//}
//
//namespace tests {
//
//template <int V>
//struct IsEven : std::false_type {};
//
//template <int V>
//  requires(V % 2 == 0)
//struct IsEven<V> : std::true_type {};
//
//static_assert(CountPredicateMatches<int, IsEven, 1, 2, 3, 4, 5>() == 2);
//
//}   // namespace tests
//
//}   // namespace detail
//
//template <typename T, template <T V> typename Predicate, T... Values>
//consteval std::pair<
//    std::array<T, detail::CountPredicateMatches<T, Predicate, Values...>()>,
//    std::array<T,
//               sizeof...(Values)
//                   - detail::CountPredicateMatches<T, Predicate, Values...>()>>
//Partition() noexcept {
//  constexpr auto NMatched =
//      detail::CountPredicateMatches<T, Predicate, Values...>();
//  constexpr auto NNotMatched = sizeof...(Values) - NMatched;
//
//  std::array<T, sizeof...(Values)> all{Values...};
//  std::array<T, NMatched> matched{};
//  std::array<T, NNotMatched> not_matched{};
//
//
//};

}   // namespace ct