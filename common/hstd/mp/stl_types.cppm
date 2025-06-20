module;

#include <array>

export module hstd:mp.stl_types;

namespace hstd {

export template <typename T>
inline constexpr auto IsArray = false;

export template <typename T, std::size_t N>
inline constexpr auto IsArray<std::array<T, N>> = true;

}   // namespace hstd