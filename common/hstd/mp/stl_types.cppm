module;

#include <array>

export module hstd:mp.stl_types;

import :mp.helpers;

namespace hstd {

export template <typename T>
inline constexpr auto IsArray = false;

export template <typename T, std::size_t N>
inline constexpr auto IsArray<std::array<T, N>> = true;

namespace concepts {

export template <typename T>
concept Array = IsArray<T>;

}

export template <typename T>
  requires concepts::Array<T>
inline constexpr auto ArraySize =
    ([]<typename E, std::size_t N>(hstd::Marker<std::array<E, N>>) {
      return N;
    })(hstd::Marker<T>());

}   // namespace hstd