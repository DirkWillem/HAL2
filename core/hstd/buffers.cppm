module;

#include <array>
#include <iterator>

export module hstd:buffers;

namespace hstd {

/**
 * @brief Fixed size circular buffer to which can only be appended, or be reset.
 * Useful when the last N elements of something need to be stored.
 *
 * This buffer implements the following operations:
 * - Append to the end.
 * - Clear the entire buffer.
 * - Iterate over the buffer.
 * - Index at arbitrary indices at the buffer.
 *
 * @tparam T Contained data type.
 * @tparam N Number of elements that can be stored.
 */
export template <typename T, std::size_t N>
class WriteOnlyCircularBuffer {
  static_assert(N > 0, "Buffer capacity must be at least one element");

 public:
  /**
   * @brief Iterator.
   */
  class ReverseIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = const T*;
    using reference         = const T&;

    explicit constexpr ReverseIterator(const T* head,
                                       const T* before_storage_begin,
                                       const T* storage_last_element,
                                       const T* ptr) noexcept
        : head{head}
        , before_storage_begin{before_storage_begin}
        , storage_last_element{storage_last_element}
        , ptr{ptr} {}

    constexpr reference operator*() const noexcept { return *ptr; }

    constexpr ReverseIterator operator++() noexcept {
      --ptr;
      if (ptr == before_storage_begin) {
        ptr = storage_last_element;
      }
      if (ptr == head) {
        ptr = nullptr;
      }
      return *this;
    }

    constexpr ReverseIterator operator++(int) noexcept {
      auto result = *this;
      --ptr;
      if (ptr == before_storage_begin) {
        ptr = storage_last_element;
      }
      if (ptr == head) {
        ptr = nullptr;
      }
      return result;
    }

    friend constexpr bool operator==(const ReverseIterator& lhs,
                                     const ReverseIterator& rhs) noexcept {
      return lhs.ptr == rhs.ptr;
    }

    friend constexpr bool operator!=(const ReverseIterator& lhs,
                                     const ReverseIterator& rhs) noexcept {
      return lhs.ptr != rhs.ptr;
    }

   private:
    const T* head;
    const T* before_storage_begin;
    const T* storage_last_element;
    const T* ptr;
  };

  /**
   * @brief Iterator.
   */
  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = const T*;
    using reference         = const T&;

    explicit constexpr Iterator(const T* head, const T* storage_begin,
                                const T* storage_end, const T* ptr) noexcept
        : head{head}
        , storage_begin{storage_begin}
        , storage_end{storage_end}
        , ptr{ptr} {}

    constexpr reference operator*() const noexcept { return *ptr; }

    constexpr Iterator operator++() noexcept {
      ptr++;
      if (ptr == storage_end) {
        ptr = storage_begin;
      }
      if (ptr == head) {
        ptr = nullptr;
      }
      return *this;
    }

    constexpr Iterator operator++(int) noexcept {
      auto result = *this;
      ptr++;
      if (ptr == storage_end) {
        ptr = storage_begin;
      }
      if (ptr == head) {
        ptr = nullptr;
      }
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
    const T* head;
    const T* storage_begin;
    const T* storage_end;
    const T* ptr;
  };

  WriteOnlyCircularBuffer() = default;

  WriteOnlyCircularBuffer(const WriteOnlyCircularBuffer& rhs)
      : storage{rhs.storage}
      , initialized{false}
      , h{nullptr}
      , nh{storage.begin()} {};
  WriteOnlyCircularBuffer& operator=(const WriteOnlyCircularBuffer& rhs) {
    if (&rhs == this) {
      return *this;
    }

    storage     = rhs.storage;
    initialized = false;
    h           = nullptr;
    nh          = storage.begin();
    return *this;
  }

  WriteOnlyCircularBuffer(WriteOnlyCircularBuffer&& rhs) noexcept
      : storage{rhs.storage}
      , initialized{false}
      , h{nullptr}
      , nh{storage.begin()} {};
  WriteOnlyCircularBuffer& operator=(WriteOnlyCircularBuffer&& rhs) noexcept {
    storage     = rhs.storage;
    initialized = false;
    h           = nullptr;
    nh          = storage.begin();
    return *this;
  }

  ~WriteOnlyCircularBuffer() = default;

  /**
   * @brief Pushes a value to the buffer.
   * @param value Value to push.
   */
  void Push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
    *nh = value;
    h   = nh;
    ++nh;

    // Wrap-around.
    if (nh == storage.end()) {
      nh          = storage.begin();
      initialized = true;
    }
  }

  /**
   * @brief Clears the buffer.
   */
  void Clear() noexcept {
    h           = nullptr;
    nh          = storage.begin();
    initialized = false;
  }

  /**
   * @brief Returns a pointer to the last-written item.
   * @return Pointer to last-written item, or \c nullptr if there is none.
   */
  const T* head() const noexcept { return h; }

  constexpr Iterator begin() const noexcept {
    if (initialized) {
      return Iterator{nh, storage.begin(), storage.end(), nh};
    } else {
      // Edge case - Empty buffer
      if (nh == storage.begin()) {
        return end();
      }

      return Iterator{nh, storage.begin(), storage.end(), storage.begin()};
    }
  }

  constexpr Iterator end() const noexcept {
    return Iterator{nh, storage.begin(), storage.end(), nullptr};
  }

  constexpr ReverseIterator rbegin() const noexcept {
    // Edge case - Empty buffer.
    if (!initialized && nh == storage.begin()) {
      return rend();
    }

    return ReverseIterator{h, std::prev(storage.begin()), &storage[N - 1], h};
  }

  constexpr ReverseIterator rend() const noexcept {
    if (initialized) {
      return ReverseIterator{h, std::prev(storage.begin()), &storage[N - 1],
                             nullptr};
    } else {
      return ReverseIterator{h, std::prev(storage.begin()), &storage[N - 1],
                             &storage[N - 1]};
    }
  }

  /**
   * @brief Returns the buffer size.
   * @return Amount of elements in the buffer.
   */
  [[nodiscard]] constexpr std::size_t size() const noexcept {
    if (initialized) {
      return N;
    }

    return nh - storage.begin();
  }

  /**
   * @brief Returns the element at index \c idx. <c>idx == 0</c> represents the
   * oldest element in the buffer, while <c>idx == size() - 1</c> represents the
   * newest.
   *
   * @param idx Index to get the element at.
   * @return Reference to requested element.
   */
  constexpr const T& operator[](std::size_t idx) const noexcept {
    // If the buffer is not filled, use regular array indexing.
    if (!initialized) {
      return storage[idx];
    }

    // Buffer is initialized. Oldest element is at nh, second-to-oldest at nh+1,
    // etc.
    const auto nh_idx = nh - storage.begin();
    return storage[(nh_idx + idx) % N];
  }

  [[nodiscard]] constexpr bool Filled() const noexcept { return initialized; }

 private:
  std::array<T, N> storage{};   //!< Underlying storage.
  bool initialized{false};      //!< Whether the buffer is fully written once.
  T*   h{nullptr};              //!< Pointer to last written item.
  T*   nh{storage.begin()};     //!< Pointer to the next written item.
};

}   // namespace hstd