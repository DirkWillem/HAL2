#pragma once

#include <stm32g4xx_ll_cordic.h>

#include <fp/fix.h>

namespace stm32g4 {

inline constexpr unsigned DefaultCordicPrecision = 6;

template <unsigned P = DefaultCordicPrecision>
  requires(P >= 1 && P <= 15)
struct Cordic {
  [[nodiscard]] static fp::Q<1, 15> Sin(fp::Q<1, 15> x) noexcept {
    LL_CORDIC_Config(CORDIC, LL_CORDIC_FUNCTION_SINE, Precision,
                     LL_CORDIC_SCALE_0, LL_CORDIC_NBWRITE_1, LL_CORDIC_NBREAD_1,
                     LL_CORDIC_INSIZE_16BITS, LL_CORDIC_OUTSIZE_16BITS);

    LL_CORDIC_WriteData(CORDIC,
                        static_cast<uint32_t>(std::bit_cast<uint16_t>(x.raw()))
                            | (OneQ1_15 << 16));
    return fp::Q<1, 15>{static_cast<int16_t>(
        std::bit_cast<int32_t>(LL_CORDIC_ReadData(CORDIC)))};
  }

  [[nodiscard]] static fp::Q<1, 31> Sin(fp::Q<1, 31> x) noexcept {
    LL_CORDIC_SetFunction(CORDIC, LL_CORDIC_FUNCTION_SINE);
    LL_CORDIC_Config(CORDIC, LL_CORDIC_FUNCTION_SINE, Precision,
                     LL_CORDIC_SCALE_0, LL_CORDIC_NBWRITE_1, LL_CORDIC_NBREAD_1,
                     LL_CORDIC_INSIZE_32BITS, LL_CORDIC_OUTSIZE_32BITS);
    LL_CORDIC_WriteData(CORDIC, x.raw());
    return fp::Q<1, 31>{std::bit_cast<int32_t>(LL_CORDIC_ReadData(CORDIC))};
  }

  [[nodiscard]] static fp::Q<1, 31> Atan2(fp::Q<1, 31> y, fp::Q<1, 31> x) {
    LL_CORDIC_Config(CORDIC, LL_CORDIC_FUNCTION_PHASE, Precision,
                     LL_CORDIC_SCALE_0, LL_CORDIC_NBWRITE_2, LL_CORDIC_NBREAD_1,
                     LL_CORDIC_INSIZE_32BITS, LL_CORDIC_OUTSIZE_32BITS);

    LL_CORDIC_WriteData(CORDIC, x.raw());
    LL_CORDIC_WriteData(CORDIC, y.raw());

    return fp::Q<1, 31>{std::bit_cast<int32_t>(LL_CORDIC_ReadData(CORDIC))};
  }

 private:
  static constexpr unsigned Precision = P << 4U;

  static constexpr uint32_t OneQ1_15 = 0x7FFF;
};

}   // namespace stm32g4
