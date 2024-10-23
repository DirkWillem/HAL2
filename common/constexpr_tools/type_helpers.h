#pragma once

#include <type_traits>

namespace ct {

template <std::size_t V>
using Size = std::integral_constant<std::size_t, V>;

template <typename T>
concept Integer = std::is_unsigned_v<T> || std::is_signed_v<T>;

template <typename T, typename U>
struct Map_t {
  using type = U;
};

template <typename T, typename U>
using Map = Map_t<T, U>::type;

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
  using Matched   = Next::Matched;
  using Unmatched = Next::Unmatched;
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

}   // namespace ct