#pragma once

namespace math {

#if defined(HAS_CMSIS_DSP)
inline constexpr bool CmsisDspAvailable = true;
#else
inline constexpr bool CmsisDspAvailable = false;
#endif

}   // namespace math
