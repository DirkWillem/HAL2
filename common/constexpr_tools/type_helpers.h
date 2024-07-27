#pragma once

#include <type_traits>

namespace ct {

template <std::size_t V>
using Size = std::integral_constant<std::size_t, V>;

}   // namespace ct