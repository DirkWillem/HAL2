module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <utility>

export module hal.stm32h5:i2c.timing;

import hal.abstract;
import hstd;

namespace stm32h5 {

constexpr auto I2cAnalogFilterDelayMin     = 50UL;
constexpr auto I2cAnalogFilterDelayMax     = 90UL;
constexpr auto I2cUseAnalogFilter          = true;
constexpr auto I2cDigitalFilterCoefficient = 0U;
constexpr auto I2cPrescMax                 = 16U;
constexpr auto I2cScldelMax                = 16U;
constexpr auto I2cSdadelMax                = 16U;
constexpr auto I2cScllMax                  = 256UL;
constexpr auto I2cSclhMax                  = 256UL;
constexpr auto Nanoseconds                 = 1'000'000'000UL;

struct I2cCharacteristic {
  uint32_t freq;      /* Frequency in Hz */
  uint32_t freq_min;  /* Minimum frequency in Hz */
  uint32_t freq_max;  /* Maximum frequency in Hz */
  uint32_t hddat_min; /* Minimum data hold time in ns */
  uint32_t vddat_max; /* Maximum data valid time in ns */
  uint32_t sudat_min; /* Minimum data setup time in ns */
  uint32_t lscl_min;  /* Minimum low period of the SCL clock in ns */
  uint32_t hscl_min;  /* Minimum high period of SCL clock in ns */
  uint32_t trise;     /* Rise time in ns */
  uint32_t tfall;     /* Fall time in ns */
  uint32_t dnf;       /* Digital noise filter coefficient */
};

/* Private Private Constants
 * ---------------------------------------------------------*/
static constexpr std::array<I2cCharacteristic, 3> I2cCharacteristics{{
    {
        .freq      = 100000,
        .freq_min  = 80000,
        .freq_max  = 120000,
        .hddat_min = 0,
        .vddat_max = 3450,
        .sudat_min = 250,
        .lscl_min  = 4700,
        .hscl_min  = 4000,
        .trise     = 0,
        .tfall     = 0,
        .dnf       = I2cDigitalFilterCoefficient,
    },
    {
        .freq      = 400000,
        .freq_min  = 320000,
        .freq_max  = 480000,
        .hddat_min = 0,
        .vddat_max = 900,
        .sudat_min = 100,
        .lscl_min  = 1300,
        .hscl_min  = 600,
        .trise     = 0,
        .tfall     = 0,
        .dnf       = I2cDigitalFilterCoefficient,
    },
    {
        .freq      = 1000000,
        .freq_min  = 800000,
        .freq_max  = 1200000,
        .hddat_min = 0,
        .vddat_max = 450,
        .sudat_min = 50,
        .lscl_min  = 500,
        .hscl_min  = 260,
        .trise     = 0,
        .tfall     = 0,
        .dnf       = I2cDigitalFilterCoefficient,
    },
}};

struct I2cTiming {
  uint32_t presc;
  uint32_t tscldel;
  uint32_t tsdadel;
  uint32_t sclh;
  uint32_t scll;
};

inline constexpr int32_t TafdelMin =
    static_cast<int32_t>(I2cUseAnalogFilter ? I2cAnalogFilterDelayMin : 0);
inline constexpr int32_t TafdelMax =
    static_cast<int32_t>(I2cUseAnalogFilter ? I2cAnalogFilterDelayMax : 0);

template <std::size_t N>
constexpr std::tuple<std::array<I2cTiming, N>, std::size_t>
ComputePrescScldelSdadel(hstd::Frequency auto clock_src_freq, uint32_t speed_idx) noexcept {
  std::array<I2cTiming, N> result{};

  uint32_t prev_presc = I2cPrescMax;

  const uint32_t clock_src_freq_hz = clock_src_freq.template As<hstd::Hz>().count;
  const uint32_t ti2cclk           = (Nanoseconds + (clock_src_freq_hz / 2U)) / clock_src_freq_hz;
  const uint32_t ti2cclk_s         = static_cast<int32_t>(ti2cclk);

  /* tDNF = DNF x tI2CCLK
     tPRESC = (PRESC+1) x tI2CCLK
     SDADEL >= {tf +tHD;DAT(min) - tAF(min) - tDNF - [3 x tI2CCLK]} / {tPRESC}
     SDADEL <= {tVD;DAT(max) - tr - tAF(max) - tDNF- [4 x tI2CCLK]} / {tPRESC}
   */

  int32_t tsdadel_min_s =
      static_cast<int32_t>(I2cCharacteristics[speed_idx].tfall)
      + static_cast<int32_t>(I2cCharacteristics[speed_idx].hddat_min) - TafdelMin
      - (static_cast<int32_t>(I2cCharacteristics[speed_idx].dnf) + 3) * ti2cclk_s;

  int32_t tsdadel_max_s = static_cast<int32_t>(I2cCharacteristics[speed_idx].vddat_max)
                          - static_cast<int32_t>(I2cCharacteristics[speed_idx].trise - TafdelMax)
                          - static_cast<int32_t>(I2cCharacteristics[speed_idx].dnf + 4) * ti2cclk_s;

  /* {[tr+ tSU;DAT(min)] / [tPRESC]} - 1 <= SCLDEL */
  const int32_t tscldel_min = static_cast<int32_t>(I2cCharacteristics[speed_idx].trise)
                              + static_cast<int32_t>(I2cCharacteristics[speed_idx].sudat_min);

  uint32_t tsdadel_min = 0;
  if (tsdadel_min_s > 0) {
    tsdadel_min = static_cast<uint32_t>(tsdadel_min_s);
  }

  uint32_t tsdadel_max = 0;
  if (tsdadel_max_s > 0) {
    tsdadel_max = static_cast<uint32_t>(tsdadel_max_s);
  }

  std::size_t idx{0};

  for (uint32_t presc = 0; presc < I2cPrescMax; presc++) {
    for (uint32_t scldel = 0; scldel < I2cScldelMax; scldel++) {
      /* TSCLDEL = (SCLDEL+1) * (PRESC+1) * TI2CCLK */
      const uint32_t tscldel = (scldel + 1U) * (presc + 1U) * ti2cclk;

      if (tscldel >= (uint32_t)tscldel_min) {
        for (uint32_t sdadel = 0; sdadel < I2cSdadelMax; sdadel++) {
          /* TSDADEL = SDADEL * (PRESC+1) * TI2CCLK */
          const uint32_t tsdadel = (sdadel * (presc + 1U)) * ti2cclk;

          if ((tsdadel >= tsdadel_min) && (tsdadel <= tsdadel_max)) {
            if (presc != prev_presc) {
              result[idx].presc   = presc;
              result[idx].tscldel = scldel;
              result[idx].tsdadel = sdadel;
              prev_presc          = presc;
              idx++;

              if (idx >= result.size()) {
                return {result, result.size()};
              }
            }
          }
        }
      }
    }
  }

  return {result, idx};
}

constexpr std::optional<I2cTiming> ComputeScllSclh(hstd::Frequency auto       clock_src_freq,
                                                   uint32_t                   speed_idx,
                                                   std::span<const I2cTiming> timings) noexcept {
  bool     found    = false;
  uint32_t ret      = 0xFFFFFFFFU;
  uint32_t ret_scll = 0;
  uint32_t ret_sclh = 0;

  const uint32_t clock_src_freq_hz = clock_src_freq.template As<hstd::Hz>().count;
  const uint32_t ti2cclk           = (Nanoseconds + (clock_src_freq_hz / 2U)) / clock_src_freq_hz;
  const uint32_t ti2cspeed         = (Nanoseconds + (I2cCharacteristics[speed_idx].freq / 2U))
                             / I2cCharacteristics[speed_idx].freq;

  const uint32_t tafdel_min = I2cUseAnalogFilter ? I2cAnalogFilterDelayMin : 0U;

  /* tDNF = DNF x tI2CCLK */
  const uint32_t dnf_delay = I2cCharacteristics[speed_idx].dnf * ti2cclk;

  const uint32_t clk_max = Nanoseconds / I2cCharacteristics[speed_idx].freq_min;
  const uint32_t clk_min = Nanoseconds / I2cCharacteristics[speed_idx].freq_max;

  uint32_t prev_error = 0xFFFFFFFF;

  for (uint32_t count = 0; count < timings.size(); count++) {
    /* tPRESC = (PRESC+1) x tI2CCLK*/
    uint32_t tpresc = (timings[count].presc + 1U) * ti2cclk;

    const auto scll_lb =
        std::max(static_cast<unsigned long>((I2cCharacteristics[speed_idx].lscl_min
                                             - (tafdel_min + dnf_delay + (2U * ti2cclk)))
                                            / tpresc),
                 std::max(static_cast<unsigned long>(2U / (timings[count].presc + 1U)), 1UL));
    for (uint32_t scll = scll_lb; scll < I2cScllMax; scll += 4) {
      const uint32_t tscl_l = tafdel_min + dnf_delay + (2U * ti2cclk) + ((scll + 1U) * tpresc);

      const auto sclh_lb =
          std::max((clk_min
                    - (2 * (tafdel_min + dnf_delay) + (4U * ti2cclk) + ((scll + 2U) * tpresc)
                       + I2cCharacteristics[speed_idx].trise + I2cCharacteristics[speed_idx].tfall))
                           / tpresc
                       - 1U,
                   (std::max(I2cCharacteristics[speed_idx].hscl_min, ti2cclk) - tafdel_min
                    - dnf_delay - (2U * ti2cclk))
                           / tpresc
                       - 1U);
      const auto sclh_ub = std::min(
          I2cSclhMax,
          static_cast<unsigned long>(
              (clk_max
               - (2 * (tafdel_min + dnf_delay) + (4U * ti2cclk) + ((scll + 2U) * tpresc)
                  + I2cCharacteristics[speed_idx].trise + I2cCharacteristics[speed_idx].tfall))
                  / tpresc
              - 1U));

      for (uint32_t sclh = sclh_lb; sclh < sclh_ub; sclh += 4) {
        /* tHIGH(min) <= tAF(min) + tDNF + 2 x tI2CCLK + [(SCLH+1) x tPRESC]
         */
        uint32_t tscl_h = tafdel_min + dnf_delay + (2U * ti2cclk) + ((sclh + 1U) * tpresc);

        /* tSCL = tf + tLOW + tr + tHIGH */
        uint32_t tscl = tscl_l + tscl_h + I2cCharacteristics[speed_idx].trise
                        + I2cCharacteristics[speed_idx].tfall;

        int32_t  error_s = (int32_t)tscl - (int32_t)ti2cspeed;
        uint32_t error   = 0;

        if (error_s >= 0) {
          error = static_cast<uint32_t>(error_s);
        } else {
          error = static_cast<uint32_t>(-error_s);
        }

        /* look for the timings with the lowest clock error */
        if (error < prev_error) {
          prev_error = error;
          ret_scll   = scll;
          ret_sclh   = sclh;
          ret        = count;
          found      = true;
        }
      }
    }
  }

  if (found) {
    auto result = timings[ret];
    result.scll = ret_scll;
    result.sclh = ret_sclh;
    return {result};
  }

  return {};
}

export consteval std::optional<uint32_t> CalculateI2cTiming(hstd::Frequency auto src_clk,
                                                            hal::I2cSpeedMode    speed) {
  switch (speed) {
  case hal::I2cSpeedMode::Standard: {
    const auto intermediate = ComputePrescScldelSdadel<128>(src_clk, 0);
    const auto timing       = ComputeScllSclh(
        src_clk, 0, std::span{std::get<0>(intermediate)}.subspan(0, std::get<1>(intermediate)));

    if (timing.has_value()) {
      const auto result_val = *timing;
      return ((result_val.presc & 0x0FU) << 28) | ((result_val.tscldel & 0x0FU) << 20)
             | ((result_val.tsdadel & 0x0FU) << 16) | ((result_val.sclh & 0xFFU) << 8)
             | ((result_val.scll & 0xFFU) << 0);
    } else {
      return {};
    }
  }
  case hal::I2cSpeedMode::Fast: {
    const auto intermediate = ComputePrescScldelSdadel<128>(src_clk, 1);
    const auto timing       = ComputeScllSclh(
        src_clk, 0, std::span{std::get<0>(intermediate)}.subspan(0, std::get<1>(intermediate)));

    if (timing.has_value()) {
      const auto result_val = *timing;
      return ((result_val.presc & 0x0FU) << 28) | ((result_val.tscldel & 0x0FU) << 20)
             | ((result_val.tsdadel & 0x0FU) << 16) | ((result_val.sclh & 0xFFU) << 8)
             | ((result_val.scll & 0xFFU) << 0);
    } else {
      return {};
    }
  }
  case hal::I2cSpeedMode::FastPlus: {
    const auto intermediate = ComputePrescScldelSdadel<128>(src_clk, 2);
    const auto timing       = ComputeScllSclh(
        src_clk, 0, std::span{std::get<0>(intermediate)}.subspan(0, std::get<1>(intermediate)));

    if (timing.has_value()) {
      const auto result_val = *timing;
      return ((result_val.presc & 0x0FU) << 28) | ((result_val.tscldel & 0x0FU) << 20)
             | ((result_val.tsdadel & 0x0FU) << 16) | ((result_val.sclh & 0xFFU) << 8)
             | ((result_val.scll & 0xFFU) << 0);
    } else {
      return {};
    }
  }
  default: std::unreachable();
  }
}

}   // namespace stm32h5