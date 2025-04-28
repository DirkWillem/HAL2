#pragma once

#include <array>
#include <concepts>
#include <type_traits>

namespace halstd {

template <typename T, typename StructType, typename FieldType>
concept FieldPointer = requires(T ptr, StructType s) {
  { s.*ptr } -> std::convertible_to<FieldType>;
};

template <std::equality_comparable auto V, typename T>
struct ValToType {
  static constexpr auto Value = V;
  using Type                  = T;
};

namespace detail {

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

}   // namespace detail

template <std::equality_comparable auto V, typename... Ms>
using MapValToType = typename detail::MapValToTypeHelper<V, Ms...>::Type;

template <std::size_t V>
using Size = std::integral_constant<std::size_t, V>;

template <typename T>
concept Integer = std::is_unsigned_v<T> || std::is_signed_v<T>;


template <typename T, typename U>
using Map = U;

namespace detail {

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

}   // namespace detail

template <template <typename> typename Predicate,
          template <typename...> typename List, typename... Ts>
using PartitionTypes =
    detail::TypePartitionHelper<Predicate, List, List<>, List<>, Ts...>;

template <typename T>
struct Marker {};

template <typename... Ts>
struct Markers {};


template <auto V>
struct ValueMarker {};

struct Empty {};

template<typename In, typename Out>
using Map = Out;

}   // namespace halstd