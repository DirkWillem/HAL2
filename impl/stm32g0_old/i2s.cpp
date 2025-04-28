#include "i2s.h"

namespace stm32g0::detail {

[[nodiscard]] constexpr uint32_t
GetHalI2sStandard(hal::I2sStandard standard) noexcept {
  switch (standard) {
  case hal::I2sStandard::Philips: return I2S_STANDARD_PHILIPS;
  case hal::I2sStandard::LeftJustified: return I2S_STANDARD_MSB;
  case hal::I2sStandard::RightJustified: return I2S_STANDARD_LSB;
  case hal::I2sStandard::PcmShortSynchroFrame: return I2S_STANDARD_PCM_SHORT;
  case hal::I2sStandard::PcmLongSynchroFrame: return I2S_STANDARD_PCM_LONG;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t
GetHalI2sDataFormat(hal::I2sDataFormat format) noexcept {
  switch (format) {
  case hal::I2sDataFormat::Bits16: return I2S_DATAFORMAT_16B;
  case hal::I2sDataFormat::Bits16On32BitFrame:
    return I2S_DATAFORMAT_16B_EXTENDED;
  case hal::I2sDataFormat::Bits24On32BitFrame: return I2S_DATAFORMAT_24B;
  case hal::I2sDataFormat::Bits32: return I2S_DATAFORMAT_32B;
  }

  std::unreachable();
}

[[nodiscard]] constexpr uint32_t
GetHalI2sClockPolarity(hal::I2sClockPolarity polarity) noexcept {
  switch (polarity) {
  case hal::I2sClockPolarity::Low: return I2S_CPOL_LOW;
  case hal::I2sClockPolarity::High: return I2S_CPOL_HIGH;
  }

  std::unreachable();
}

void SetupI2s(I2S_HandleTypeDef& hi2s, I2sId id,
                      I2sAudioFrequency frequency, hal::I2sStandard standard,
                      hal::I2sDataFormat    data_format,
                      hal::I2sClockPolarity clock_polarity) noexcept {
  EnableSpiClk(GetSpiForI2s(id));

  hi2s.Instance = GetI2sPointer(id);
  hi2s.Init     = {
          .Mode       = I2S_MODE_MASTER_TX,
          .Standard   = GetHalI2sStandard(standard),
          .DataFormat = GetHalI2sDataFormat(data_format),
          .MCLKOutput = I2S_MCLKOUTPUT_DISABLE,
          .AudioFreq  = static_cast<uint32_t>(frequency),
          .CPOL       = GetHalI2sClockPolarity(clock_polarity),
  };

  HAL_I2S_Init(&hi2s);



}

}   // namespace stm32g0::detail
