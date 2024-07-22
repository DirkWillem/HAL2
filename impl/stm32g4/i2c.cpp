#include "i2c.h"

namespace stm32g4::detail {

[[nodiscard]] constexpr uint32_t
ToHalAddressingMode(hal::I2cAddressLength addr_length) noexcept {
  switch (addr_length) {
  case hal::I2cAddressLength::Bits7: return I2C_ADDRESSINGMODE_7BIT;
  case hal::I2cAddressLength::Bits10: return I2C_ADDRESSINGMODE_10BIT;
  default: std::unreachable();
  }
}

inline void EnableI2cClk(I2cId id) {
  switch (id) {
  case I2cId::I2c1: __HAL_RCC_I2C1_CLK_ENABLE(); break;
  case I2cId::I2c2: __HAL_RCC_I2C2_CLK_ENABLE(); break;
  case I2cId::I2c3: __HAL_RCC_I2C3_CLK_ENABLE(); break;
  case I2cId::I2c4: __HAL_RCC_I2C4_CLK_ENABLE(); break;
  default: std::unreachable();
  }
}

[[nodiscard]] constexpr IRQn_Type GetEventIrqn(I2cId id) noexcept {
  switch (id) {
  case I2cId::I2c1: return I2C1_EV_IRQn;
  case I2cId::I2c2: return I2C2_EV_IRQn;
  case I2cId::I2c3: return I2C3_EV_IRQn;
  case I2cId::I2c4: return I2C4_EV_IRQn;
  default: std::unreachable();
  }
}

[[nodiscard]] constexpr IRQn_Type GetErrorIrqn(I2cId id) noexcept {
  switch (id) {
  case I2cId::I2c1: return I2C1_ER_IRQn;
  case I2cId::I2c2: return I2C2_ER_IRQn;
  case I2cId::I2c3: return I2C3_ER_IRQn;
  case I2cId::I2c4: return I2C4_ER_IRQn;
  default: std::unreachable();
  }
}

void SetupI2c(I2cId id, I2C_HandleTypeDef& hi2c,
              hal::I2cAddressLength addr_length, uint32_t timingr) noexcept {
  EnableI2cClk(id);

  hi2c.Instance = GetI2cPointer(id);
  hi2c.Init     = {
          .Timing           = timingr,
          .OwnAddress1      = 0,
          .AddressingMode   = ToHalAddressingMode(addr_length),
          .DualAddressMode  = I2C_DUALADDRESS_DISABLE,
          .OwnAddress2      = 0,
          .OwnAddress2Masks = I2C_OA2_NOMASK,
          .GeneralCallMode  = I2C_GENERALCALL_DISABLE,
          .NoStretchMode    = I2C_NOSTRETCH_DISABLE,
  };

  if (HAL_I2C_Init(&hi2c) != HAL_OK) {
    return;
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
    return;
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c, 0) != HAL_OK) {
    return;
  }
}

void EnableI2cInterrupts(I2cId id) noexcept {
  auto irqn = GetEventIrqn(id);
  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);

  irqn = GetErrorIrqn(id);
  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);
}

}   // namespace stm32g4::detail