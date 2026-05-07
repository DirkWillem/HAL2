module;

#include <array>
#include <concepts>
#include <optional>

// Required for __disable_irq / __enable_irq.
#include <cmsis_compiler_cpp.h>

export module seq.port.queue.critical_section_queue;

import hstd;

namespace seq::port {

/**
 * @brief MPSC queues that prevents race conditions by disabling interrupts during pushing.
 * @tparam T Queue element type.
 * @tparam N Queue size, must be a power of two.
 */
export template <typename T, std::size_t N>
  requires(hstd::IsPowerOf2(N))
class CriticalSectionQueue {
 public:
  /**
   * @brief Pushes a value to the queue.
   * @param value Value to push.
   * @return Whether the value was successfully pushed.
   */
  bool Push(const T& value) {
    __disable_irq();

    const auto next = (head + 1) & (N - 1);
    if (next == tail) {
      __enable_irq();
      return false;
    }

    buffer[head] = value;
    head         = next;
    __enable_irq();

    return true;
  }

  /**
   * @brief Pops an element from the queue.
   * @return Popped element, or \c std::nullopt if the queue was empty.
   */
  std::optional<T> Pop() {
    if (head == tail) {
      return std::nullopt;
    }

    const auto result = buffer[tail];
    tail              = (tail) & (N - 1);
    return result;
  }

  /**
   * @brief Empties the queue and calls the given handler on every element in the queue in insertion
   * order.
   * @param handler Function that is called for every element in the queue.
   */
  void ReadAll(std::invocable<const T&> auto handler) {
    while (head != tail) {
      handler(buffer[tail]);
      tail = (tail + 1) & (N - 1);
    }
  }

 private:
  std::size_t      head{0};    //!< Queue head (write position).
  std::size_t      tail{0};    //!< Queue tail (read position).
  std::array<T, N> buffer{};   //!< Underlying storage for the queue.
};

}   // namespace seq::port