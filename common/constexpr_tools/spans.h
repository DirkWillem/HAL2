#include <span>

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

}   // namespace ct