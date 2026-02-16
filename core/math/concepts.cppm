module;

#include <concepts>
#include <cstdint>

export module math:concepts;

namespace math::concepts {

export template <typename R>
concept UnsignedInteger =
    std::is_same_v<R, uint8_t> || std::is_same_v<R, uint16_t>
    || std::is_same_v<R, uint32_t> || std::is_same_v<R, uint64_t>;

export template <typename R>
concept SignedInteger =
    std::is_same_v<R, int8_t> || std::is_same_v<R, int16_t>
    || std::is_same_v<R, int32_t> || std::is_same_v<R, int64_t>;

export template <typename R>
concept Integer = UnsignedInteger<R> || SignedInteger<R>;

export template <typename R>
concept Real = std::floating_point<R>;

export template <typename T>
concept Number = Integer<T> || Real<T>;

}   // namespace math::concepts