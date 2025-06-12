module;

#include <bit>
#include <iterator>
#include <span>
#include <string_view>
#include <utility>

export module hstd:spans;

import :endian;

namespace hstd {

/**
 * Returns whether a span is a sub-span of another span
 * @tparam T Span type
 * @param sub Subspan
 * @param of Containing span
 * @return Whether `of` contains `sub`
 */
export template <typename T>
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
export template <typename T>
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
export template <typename T>
constexpr std::span<T> MergeContiguousSpans(std::span<T> a,
                                            std::span<T> b) noexcept {
  if (!AreContiguousSpans(a, b)) {
    std::unreachable();
  }

  return {a.begin(), b.end()};
}

export template <typename T>
concept ByteLike = std::is_same_v<std::decay_t<T>, std::byte>
                   || std::is_same_v<std::decay_t<T>, unsigned char>;

export template <ByteLike TOut, typename TIn>
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

export template <ByteLike TOut, typename TIn>
/**
 * Returns a byte view over the data contained in a span
 * @tparam TOut Output type, must be a type that is able to access the raw
 *   memory representation of an object (std::byte, unsigned char)
 * @tparam TIn Input type
 * @param in Input span
 * @return Byte view over the passed span
 */
std::span<const TOut> ReinterpretSpan(std::span<TIn> in) noexcept {
  return std::span{
      reinterpret_cast<const TOut*>(in.data()),
      in.size() * (sizeof(TIn) / sizeof(TOut)),
  };
}

template <ByteLike TOut>
/**
 * Returns a byte view over the data contained in a string view
 * @tparam TOut Output type, must be a type that is able to access the raw
 *   memory representation of an object (std::byte, unsigned char)
 * @param in Input string view
 * @return Byte view over the passed span
 */
std::span<const TOut> ReinterpretSpan(std::string_view in) noexcept {
  return std::span{reinterpret_cast<const TOut*>(in.data()), in.size()};
}

export template <ByteLike TOut, typename TIn>
std::span<TOut> MutByteViewOver(TIn& in) {
  return std::span{reinterpret_cast<TOut*>(&in), sizeof(in)};
}

export template <typename T, ByteLike Underlying,
                 std::endian Endianness = std::endian::native>
  requires std::is_trivially_copyable_v<T>
/**
 * Class that provides a read-only view over a span of bytes that is cast
 * bitwise to another representation upon iterating or indexing
 * @tparam T Type that is being cast to
 * @tparam Underlying Underlying byte(-like) type
 */
class BitCastSpan {
 public:
  using element_type = T;
  using value_type   = std::remove_cv_t<T>;

  /**
   * Iterator type
   */
  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = const T*;
    using reference         = T;

    explicit constexpr Iterator(const std::byte* ptr) noexcept
        : ptr{ptr} {
      // Ensure alignment of
    }

    constexpr reference operator*() const noexcept {
      alignas(T) std::array<Underlying, sizeof(T)> buf{};
      std::memcpy(buf.data(), ptr, sizeof(T));

      if constexpr (Endianness == std::endian::native) {
        return std::bit_cast<T>(buf);
      } else {
        return ConvertToEndianness<Endianness>(std::bit_cast<T>(buf));
      }
    }

    constexpr Iterator& operator++() noexcept {
      ptr += sizeof(T);
      return *this;
    }

    constexpr Iterator operator++(int) noexcept {
      auto result = *this;
      ptr += sizeof(T);
      return result;
    }

    friend constexpr bool operator==(const Iterator& lhs,
                                     const Iterator& rhs) noexcept {
      return lhs.ptr == rhs.ptr;
    }

    friend constexpr bool operator!=(const Iterator& lhs,
                                     const Iterator& rhs) noexcept {
      return lhs.ptr != rhs.ptr;
    }

   private:
    const std::byte* ptr;
  };

  constexpr BitCastSpan() noexcept                                = default;
  constexpr BitCastSpan(const BitCastSpan&) noexcept              = default;
  constexpr BitCastSpan(BitCastSpan&&) noexcept                   = default;
  constexpr BitCastSpan& operator=(const BitCastSpan&) & noexcept = default;
  constexpr BitCastSpan& operator=(BitCastSpan&&) & noexcept      = default;
  constexpr ~BitCastSpan()                                        = default;

  /**
   * Constructor
   * @param data View over the bytes to construct the BitCastSpan over
   */
  explicit constexpr BitCastSpan(std::span<const Underlying> data)
      : data{data} {
    if (data.size() % sizeof(T) != 0) {
      std::unreachable();
    }
  }

  /**
   * Indexing operator
   * @param index Index to read the element at
   * @return Element
   */
  constexpr T operator[](std::size_t index) const noexcept {
    if (index > data.size() / sizeof(T)) {
      std::unreachable();
    }

    alignas(T) std::array<Underlying, sizeof(T)> buf{};
    std::memcpy(buf.data(), data.subspan(index * sizeof(T), sizeof(T)).data(),
                sizeof(T));

    if constexpr (Endianness == std::endian::native) {
      return std::bit_cast<T>(buf);
    } else {
      return ConvertToEndianness<Endianness>(std::bit_cast<T>(buf));
    }
  }

  /**
   * Returns the number of elements in the span
   * @return Number of elements in the span
   */
  [[nodiscard]] constexpr std::size_t size() const noexcept {
    return data.size() / sizeof(T);
  }

  /**
   * Returns an iterator pointing to the begin of the span
   * @return Begin iterator
   */
  constexpr Iterator begin() const noexcept { return Iterator{data.data()}; }

  /**
   * Returns an iterator pointing to the end of the span
   * @return Begin iterator
   */
  constexpr Iterator end() const noexcept {
    return Iterator{data.data() + data.size()};
  }

 private:
  std::span<const Underlying> data{};
};

}   // namespace hstd
