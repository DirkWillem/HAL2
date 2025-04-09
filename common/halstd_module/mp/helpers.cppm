module;

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

export module hstd:mp.helpers;

export namespace hstd {

/**
 * Maps a NTTP to a type
 * @tparam V Value to map from
 * @tparam T Type to map to
 */
template <std::equality_comparable auto V, typename T>
struct ValToType {
  static constexpr auto Value = V;
  using Type                  = T;
};

}   // namespace halstd

namespace hstd {

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

}   // namespace halstd

export namespace hstd {

/** Concept for a field pointer for a given StructType and FieldType */
template <typename T, typename StructType, typename FieldType>
concept FieldPointer = requires(T ptr, StructType s) {
  { s.*ptr } -> std::convertible_to<FieldType>;
};

template <typename T>
concept Integer = std::is_unsigned_v<T> || std::is_signed_v<T>;

template <std::equality_comparable auto V, typename... Ms>
using MapValToType = typename MapValToTypeHelper<V, Ms...>::Type;

template <std::size_t V>
using Size = std::integral_constant<std::size_t, V>;

template <template <typename> typename Predicate,
          template <typename...> typename List, typename... Ts>
using PartitionTypes =
    TypePartitionHelper<Predicate, List, List<>, List<>, Ts...>;

template <typename T>
struct Marker {};

template <typename... Ts>
struct Markers {};

template <auto V>
struct ValueMarker {};

struct Empty {};

template <typename In, typename Out>
using Map = Out;

template <auto V>
constexpr bool IsConstantExpression() {
  return true;
}

/**
 * Performs a compile time assertion. When check is false, the behavior is
 * undefined
 * @param check Check to perform
 * @param message Diagnostic message
 */
consteval void Assert(bool                              check,
                      [[maybe_unused]] std::string_view message) noexcept {
  if (!check) {
    std::unreachable();
  }
}


}   // namespace halstd
