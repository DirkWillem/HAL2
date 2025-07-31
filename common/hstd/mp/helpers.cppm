module;

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

export module hstd:mp.helpers;

namespace hstd {

/**
 * Maps a NTTP to a type
 * @tparam V Value to map from
 * @tparam T Type to map to
 */
export template <std::equality_comparable auto V, typename T>
struct ValToType {
  static constexpr auto Value = V;
  using Type                  = T;
};

template <std::equality_comparable auto V, typename... Ms>
struct MapValToTypeHelper;

template <std::equality_comparable auto                                 V,
          std::equality_comparable_with<std::decay_t<decltype(V)>> auto Vc,
          typename Tc, typename... Rest>
struct MapValToTypeHelper<V, ValToType<Vc, Tc>, Rest...> {
  using Type =
      std::conditional_t<(V == Vc), Tc,
                         typename MapValToTypeHelper<V, Rest...>::Type>;
};

template <template <typename> typename Predicate,
          template <typename...> typename List, typename Matched,
          typename Unmatched, typename... Ts>
struct TypePartitionHelper {};

template <template <typename> typename Predicate,
          template <typename...> typename List, typename... MatchedTypes,
          typename... UnmatchedTypes, typename TCur, typename... TRest>
struct TypePartitionHelper<Predicate, List, List<MatchedTypes...>,
                           List<UnmatchedTypes...>, TCur, TRest...> {
 private:
  using NextMatched =
      std::conditional_t<Predicate<TCur>::value, List<MatchedTypes..., TCur>,
                         List<MatchedTypes...>>;
  using NextUnmatched =
      std::conditional_t<Predicate<TCur>::value, List<UnmatchedTypes...>,
                         List<UnmatchedTypes..., TCur>>;

  using Next = TypePartitionHelper<Predicate, List, NextMatched, NextUnmatched,
                                   TRest...>;

 public:
  using Matched   = typename Next::Matched;
  using Unmatched = typename Next::Unmatched;
};

template <template <typename> typename Predicate,
          template <typename...> typename List, typename... MatchedTypes,
          typename... UnmatchedTypes>
struct TypePartitionHelper<Predicate, List, List<MatchedTypes...>,
                           List<UnmatchedTypes...>> {
  using Matched   = List<MatchedTypes...>;
  using Unmatched = List<UnmatchedTypes...>;
};

/** Concept for a field pointer for a given StructType and FieldType */
export template <typename T, typename StructType, typename FieldType>
concept FieldPointer = requires(T ptr, StructType s) {
  { s.*ptr } -> std::convertible_to<FieldType>;
};

export template <typename T>
concept Integer = std::is_unsigned_v<T> || std::is_signed_v<T>;

export template <std::equality_comparable auto V, typename... Ms>
using MapValToType = typename MapValToTypeHelper<V, Ms...>::Type;

export template <std::size_t V>
using Size = std::integral_constant<std::size_t, V>;

export template <template <typename> typename Predicate,
                 template <typename...> typename List, typename... Ts>
using PartitionTypes =
    TypePartitionHelper<Predicate, List, List<>, List<>, Ts...>;

export template <typename>
struct Marker {};

export template <typename...>
struct Markers {};

export template <auto>
struct ValueMarker {};

export struct Empty {};

export template <typename, typename Out>
using Map = Out;

export template <auto>
constexpr bool IsConstantExpression() {
  return true;
}

/**
 * Performs a compile time assertion. When check is false, the behavior is
 * undefined
 * @param check Check to perform
 * @param message Diagnostic message
 */
export consteval void
Assert(bool check, [[maybe_unused]] std::string_view message) noexcept {
  if (!check) {
    std::unreachable();
  }
}

export template <typename TIn, typename TOut, std::size_t N>
[[nodiscard]] consteval TOut
StaticMap(std::equality_comparable_with<TIn> auto value,
          std::array<std::pair<TIn, TOut>, N>     mapping) {
  for (const auto [in, out] : mapping) {
    if constexpr (std::is_integral_v<TIn> && std::is_integral_v<TOut>
                  && !std::is_same_v<TIn, TOut>) {
      if (value == static_cast<TOut>(in)) {
        return out;
      }
    } else {
      if (value == in) {
        return out;
      }
    }
  }

  std::unreachable();
}

export template <std::size_t N>
struct StaticString {
  static constexpr auto Size = N;

  constexpr StaticString(const char (&str)[N]) noexcept {
    for (std::size_t i = 0; i < N; ++i) {
      value[i] = str[i];
    }
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept { return N; }

  constexpr auto operator<=>(const StaticString&) const noexcept = default;

  explicit(false) constexpr operator std::string_view() const noexcept {
    return std::string_view{value.data(), N - 1};
  }

  std::array<char, N> value;
};

export template <std::size_t N>
StaticString(const char (&)[N]) -> StaticString<N>;

namespace concepts {

template <typename T>
inline constexpr bool IsStaticString = false;

template <std::size_t N>
inline constexpr bool IsStaticString<::hstd::StaticString<N>> = true;

export template <typename T>
concept StaticString = IsStaticString<T>;

export template <typename Impl>
concept ToStringView =
    requires(const Impl& c_impl) { static_cast<std::string_view>(c_impl); };

}   // namespace concepts

/**
 * Invokes a callable with a ValueMarker for each number in the range [0, N]
 * @tparam N Number up to which to count
 * @param fn Callable to invoke
 */
export template <std::size_t N>
void InvokeForIndexSequence(
    std::invocable<hstd::ValueMarker<std::size_t{0}>> auto&& fn) {
  ([&fn]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
    (..., fn(ValueMarker<Idxs>{}));
  })(std::make_index_sequence<N>());
}

}   // namespace hstd
