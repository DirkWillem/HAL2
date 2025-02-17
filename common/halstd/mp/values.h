#pragma once

#include <array>
#include <concepts>
#include <optional>
#include <type_traits>

namespace halstd {

template <typename T, typename V>
concept ValueMapping = requires {
  { T::Map(std::declval<V>()) };
};

template <typename T, ValueMapping<T> VM>
using ValueMappingResult = decltype(VM::Map(std::declval<T>()));

template <typename T, T... Vals>
/**
 * Represents a compile-time list of values of the same type
 * @tparam T Value type
 * @tparam Vals Values
 */
struct Values {
  /** Array type that can store the values in the value list */
  using Array = std::array<T, sizeof...(Vals)>;

  /** Number of valeus in the list */
  static constexpr std::size_t Count = sizeof...(Vals);

  /** Returns the list of values as an array */
  static consteval Array ToArray() noexcept { return {Vals...}; }

  template <ValueMapping<T> VM>
  using Map = Values<ValueMappingResult<T, VM>, VM::Map(Vals)...>;

  /**
   * Returns whether all values in the list are equal
   * @return Whether all values are equal
   */
  static consteval bool AreEqual() noexcept
    requires(std::equality_comparable<T>)
  {
    const auto arr = ToArray();
    if (arr.size() == 0) {
      return true;
    }

    for (auto& el : arr) {
      if (el != arr[0]) {
        return false;
      }
    }

    return true;
  }

  /**
   * In case where all values in the list are equal, returns the single value in
   *   the list
   * @return Single value in the list
   */
  static consteval T SingleValue() noexcept {
    static_assert(sizeof...(Vals) > 0 && AreEqual(),
                  "SingleValue is only defined for Values where there is at "
                  "least one value, and all values are equal");

    return ToArray()[0];
  }

  /**
   * Returns whether all values in the list are unique
   * @return Whether all values are unique
   */
  static consteval bool AreUnique() noexcept
    requires(std::equality_comparable<T>)
  {
    constexpr auto arr = ToArray();
    for (std::size_t i = 0; i < Count; i++) {
      for (std::size_t j = i + 1; j < Count; j++) {
        if (arr[i] == arr[j]) {
          return false;
        }
      }
    }

    return true;
  }

  /**
   * Finds a value in the list by a predicate
   * @param predicate Predicate to find value by
   * @return Found value, or std::nullopt when not found
   */
  [[nodiscard]] static constexpr std::optional<T>
  FindBy(std::predicate<const T&> auto predicate) noexcept {
    constexpr auto arr = ToArray();

    for (const auto& v : arr) {
      if (predicate(v)) {
        return v;
      }
    }

    return {};
  }

  [[nodiscard]] static constexpr std::size_t
  GetIndexBy(std::predicate<const T&> auto predicate) noexcept {
    constexpr auto arr = ToArray();

    for (std::size_t i = 0; i < Count; i++) {
      if (predicate(arr[i])) {
        return i;
      }
    }

    std::unreachable();
  }

  [[nodiscard]] static consteval bool Contains(T value) noexcept
    requires(std::equality_comparable<T>)
  {
    constexpr auto arr = ToArray();

    for (std::size_t i = 0; i < Count; i++) {
      if (arr[i] == value) {
        return true;
      }
    }

    return false;
  }

  [[nodiscard]] static consteval std::size_t GetIndex(T value) noexcept
    requires(std::equality_comparable<T>)
  {
    constexpr auto arr = ToArray();

    for (std::size_t i = 0; i < Count; i++) {
      if (arr[i] == value) {
        return i;
      }
    }

    std::unreachable();
  }

  [[nodiscard]] static consteval T GetByIndex(std::size_t idx) noexcept {
    constexpr auto arr = ToArray();

    if (idx < arr.size()) {
      return arr[idx];
    }

    std::unreachable();
  }
};

namespace detail {

template <typename T, typename E>
struct IsValuesHelper : std::false_type {};

template <typename T, typename E, E... Vals>
struct IsValuesHelper<Values<E, Vals...>, T> : std::true_type {};

}   // namespace detail

template <typename T, typename E>
concept IsValues = detail::IsValuesHelper<T, E>::value;

namespace detail {

template <typename T, typename... Ls>
struct MergeValuesHelper;

template <typename T, T... Vals>
struct MergeValuesHelper<T, Values<T, Vals...>> {
  using Result = Values<T, Vals...>;
};

template <typename T, T... Vals1, T... Vals2>
struct MergeValuesHelper<T, Values<T, Vals1...>, Values<T, Vals2...>> {
  using Result = Values<T, Vals1..., Vals2...>;
};

template <typename T, T... Vals1, typename... TRest>
struct MergeValuesHelper<T, Values<T, Vals1...>, TRest...> {
  using Result = MergeValuesHelper<
      Values<T, Vals1...>,
      typename MergeValuesHelper<T, TRest...>::Result>::Result;
};

template <std::size_t I0, typename T, typename... Ls>
struct MergeValuesWithIndexHelper;

template <std::size_t I0, typename T, T... Vals>
struct MergeValuesWithIndexHelper<I0, T, Values<T, Vals...>> {
  using Result = Values<std::pair<std::size_t, T>, std::make_pair(I0, Vals)...>;
};

template <std::size_t I0, typename T, T... Vals1, T... Vals2>
struct MergeValuesWithIndexHelper<I0, T, Values<T, Vals1...>,
                                  Values<T, Vals2...>> {
  using Result = Values<std::pair<std::size_t, T>, std::make_pair(I0, Vals1)...,
                        std::make_pair(I0 + 1, Vals2)...>;
};

template <std::size_t I0, typename T, T... Vals1, typename... TRest>
struct MergeValuesWithIndexHelper<I0, Values<T, Vals1...>, TRest...> {
  using Result = MergeValuesHelper<
      Values<std::pair<std::size_t, T>, std::make_pair(I0, Vals1)...>,
      typename MergeValuesWithIndexHelper<I0 + 1, T, TRest...>::Result>::Result;
};

}   // namespace detail

template <typename T, typename... Vs>
/**
 * Merges a list of Values
 * @tparam T Element type of each list
 * @tparam Vs List of Values
 */
using MergeValues = detail::MergeValuesHelper<T, Vs...>::Result;

template <typename T, typename... Vs>
/**
 * Merges a list of Values, adding the index of each list in a pair with the
 *   value from the list
 * @tparam T Element type in the source lists
 * @tparam Vs List of values
 */
using MergeValuesWithIndex =
    detail::MergeValuesWithIndexHelper<0, T, Vs...>::Result;

}   // namespace ct