#pragma once

#include <span>
#include <utility>

namespace ct {

/**
 * Returns whether a span is a sub-span of another span
 * @tparam T Span type
 * @param sub Subspan
 * @param of Containing span
 * @return Whether `of` contains `sub`
 */
template <typename T>
[[nodiscard]] constexpr bool IsSubspan(std::span<T> sub,
                                       std::span<T> of) noexcept {
  return sub.data() >= of.data() && sub.end() <= of.end();
}

/**
 * Returns whether two spans are contiguous in memory
 * @tparam T Span type
 * @param a First span
 * @param b Second span
 * @return Whether a and b are contiguous
 */
template <typename T>
constexpr bool AreContiguousSpans(std::span<T> a, std::span<T> b) noexcept {
  return a.end() == b.begin();
}

/**
 * Merges two contiguous spans.
 * @tparam T Span type
 * @param a First span
 * @param b Second span
 * @return Merged span
 * @note If a and b are not contiguous, the behavior is undefined
 */
template <typename T>
constexpr std::span<T> MergeContiguousSpans(std::span<T> a,
                                            std::span<T> b) noexcept {
  if (!AreContiguousSpans(a, b)) {
    std::unreachable();
  }

  return {a.begin(), b.end()};
}

template <typename T>
concept ByteLike = std::is_same_v<std::decay_t<T>, std::byte>
                   || std::is_same_v<std::decay_t<T>, unsigned char>;

template <ByteLike TOut, typename TIn>
/**
 * Returns a byte view over the data contained in a span
 * @tparam TOut Output type, must be a type that is able to access the raw
 *   memory representation of an object (std::byte, unsigned char)
 * @tparam TIn Input type
 * @param in Input span
 * @return Byte view over the passed span
 */
std::span<TOut> ReinterpretSpanMut(std::span<TIn> in) noexcept {
  return std::span{
      reinterpret_cast<TOut*>(in.data()),
      in.size() * (sizeof(TIn) / sizeof(TOut)),
  };
}

template <ByteLike TOut, typename TIn>
/**
 * Returns a byte view over the data contained in a span
 * @tparam TOut Output type, must be a type that is able to access the raw
 *   memory representation of an object (std::byte, unsigned char)
 * @tparam TIn Input type
 * @param in Input span
 * @return Byte view over the passed span
 */
std::span<const TOut> ReinterpretSpan(std::span<const TIn> in) noexcept {
  return std::span{
      reinterpret_cast<const TOut*>(in.data()),
      in.size() * (sizeof(TIn) / sizeof(TOut)),
  };
}

template <ByteLike TOut, typename TIn>
std::span<TOut> MutByteViewOver(TIn& in) {
  return std::span{reinterpret_cast<TOut*>(&in), sizeof(in)};
}

}   // namespace ct