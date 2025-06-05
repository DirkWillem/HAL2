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

  eg.Wait(std::declval<uint32_t>(), std::declval<std::chrono::milliseconds>(),
          std::declval<bool>(), std::declval<bool>());
};

export template <typename T>
concept Rtos = requires { TaskRef<std::decay_t<typename T::TaskRef>>; };

}   // namespace rtos::concepts
