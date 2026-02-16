module;

#include <type_traits>

export module rtos.check;


namespace rtos {

export struct FreeRtosMarker {};

export template<typename T>
struct RtosUsedMarker : std::false_type {};

export template<typename T>
consteval bool IsRtosUsed() noexcept {
  return RtosUsedMarker<T>::value;
}

}

