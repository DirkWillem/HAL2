#pragma once

namespace stm32h7 {

enum class Core { Cm4, Cm7 };

#if defined(CORE_CM4)
inline constexpr auto CurrentCore = Core::Cm4;
#elif defined(CORE_CM7)
inline constexpr auto CurrentCore = Core::Cm7;
#endif

}   // namespace stm32h7