#include "spi.h"

namespace stm32h7::detail {

[[nodiscard]] constexpr uint32_t ToHalDataSize(unsigned size) noexcept {
  return (size - 1);
}

static_assert(ToHalDataSize(4) == SPI_DATASIZE_4BIT);
static_assert(ToHalDataSize(16) == SPI_DATASIZE_16BIT);

[[nodiscard]] constexpr uint32_t
ToHalMasterDirection(const hal::SpiTransmissionType tt) noexcept {
  switch (tt) {
  case hal::SpiTransmissionType::FullDuplex: return SPI_DIRECTION_2LINES;
  case hal::SpiTransmissionType::HalfDuplex: return SPI_DIRECTION_1LINE;
  case hal::SpiTransmissionType::TxOnly: return SPI_DIRECTION_2LINES;
  case hal::SpiTransmissionType::RxOnly: return SPI_DIRECTION_2LINES_RXONLY;
  }

  std::unreachable();
}

inline void EnableSpiClk(const SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: __HAL_RCC_SPI1_CLK_ENABLE(); break;
  case SpiId::Spi2: __HAL_RCC_SPI2_CLK_ENABLE(); break;
  case SpiId::Spi3: __HAL_RCC_SPI3_CLK_ENABLE(); break;
  case SpiId::Spi4: __HAL_RCC_SPI4_CLK_ENABLE(); break;
  case SpiId::Spi5: __HAL_RCC_SPI5_CLK_ENABLE(); break;
  case SpiId::Spi6: __HAL_RCC_SPI6_CLK_ENABLE(); break;
  default: break;
  }
}

[[nodiscard]] constexpr IRQn_Type GetSpiIrqn(SpiId id) noexcept {
  switch (id) {
  case SpiId::Spi1: return SPI1_IRQn;
  case SpiId::Spi2: return SPI2_IRQn;
  case SpiId::Spi3: return SPI3_IRQn;
  case SpiId::Spi4: return SPI4_IRQn;
  case SpiId::Spi5: return SPI5_IRQn;
  case SpiId::Spi6: return SPI6_IRQn;
  default: std::unreachable();
  }
}

void EnableSpiInterrupt(const SpiId id) noexcept {
  const auto irqn = GetSpiIrqn(id);
  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);
}

void SetupSpiMaster(const SpiId id, SPI_HandleTypeDef& hspi,
                    SpiBaudPrescaler baud_prescaler, const unsigned data_size,
                    const hal::SpiTransmissionType transmission_type) noexcept {
  EnableSpiClk(id);

  hspi.Instance = GetSpiPointer(id);
  hspi.Init     = {
          .Mode              = SPI_MODE_MASTER,
          .Direction         = ToHalMasterDirection(transmission_type),
          .DataSize          = ToHalDataSize(data_size),
          .CLKPolarity       = SPI_POLARITY_LOW,
          .CLKPhase          = SPI_PHASE_1EDGE,
          .NSS               = SPI_NSS_SOFT,
          .BaudRatePrescaler = static_cast<uint32_t>(baud_prescaler),
          .FirstBit          = SPI_FIRSTBIT_MSB,
          .TIMode            = SPI_TIMODE_DISABLE,
          .CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
          .CRCPolynomial     = 7,
          .CRCLength         = SPI_CRC_LENGTH_DATASIZE,
          .NSSPMode          = SPI_NSS_PULSE_DISABLE,
  };

  HAL_SPI_Init(&hspi);
}

}   // namespace stm32h7::detail