module;

#include <chrono>
#include <cstdint>
#include <utility>

export module rtos.concepts;

namespace rtos::concepts {

export template <typename Impl>
concept TaskRef = requires(Impl& impl) {
  impl.NotifySetBits(std::declval<uint32_t>());
  impl.NotifySetBitsFromInterrupt(std::declval<uint32_t>());
};

export template <typename EG>
concept EventGroup = requires(EG& eg) {
  eg.SetBits(std::declval<uint32_t>());
  eg.SetBitsFromInterrupt(std::declval<uint32_t>());

  { eg.ClearBits(std::declval<uint32_t>()) } -> std::convertible_to<uint32_t>;
  { eg.ReadBits() } -> std::convertible_to<uint32_t>;

  eg.Wait(std::declval<uint32_t>(), std::declval<std::chrono::milliseconds>(),
          std::declval<bool>(), std::declval<bool>());
};

// struct Callable {
//   void operator()() noexcept {}
// };
//
// template <template <typename> typename Task>
// class TaskImpl
//     : Callable
//     , Task<Callable> {
//  public:
//   TaskImpl(const char* name)
//       : Task<Callable>{name} {}
// };

template <typename Bsp>
class TaskImpl : public Bsp::template Task<TaskImpl<Bsp>> {
 public:
  TaskImpl(const char* name)
      : Bsp::template Task<TaskImpl<Bsp>>{name} {}

  void operator()() noexcept {}
};

export template <typename T>
concept Rtos = requires {
  // TaskRef<std::decay_t<typename T::TaskRef>>;
  EventGroup<std::decay_t<typename T::EventGroup>>;

  { T::MiniStackSize } -> std::convertible_to<std::size_t>;
  { T::SmallStackSize } -> std::convertible_to<std::size_t>;
  { T::MediumStackSize } -> std::convertible_to<std::size_t>;
  { T::LargeStackSize } -> std::convertible_to<std::size_t>;
  { T::ExtraLargeStackSize } -> std::convertible_to<std::size_t>;

  typename T::template Task<TaskImpl<T>>;
  typename T::template Task<TaskImpl<T>, T::MiniStackSize>;

  typename T::System;

  { TaskImpl<T>{"MyTask"} };
};

}   // namespace rtos::concepts
