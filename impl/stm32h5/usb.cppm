module;

#include <stm32h5xx_hal.h>

export module hal.stm32h5:usb;

import hstd;

import :nvic;

namespace stm32h5 {

/**
 * @brief Initializes the USB peripheral as a USB device. The STM32H5 HAL
 * library does not implement USB functionality, for this, TinyUSB should be
 * used.
 *
 * @tparam Impl Implementation type, used to derive USB interrupt priority.
 */
export template <typename Impl = hstd::Empty>
void InitializeUsbDevice() {
  RCC_PeriphCLKInitTypeDef clk_init = {};
  clk_init.PeriphClockSelection     = RCC_PERIPHCLK_USB;
  clk_init.UsbClockSelection        = RCC_USBCLKSOURCE_HSI48;

  HAL_RCCEx_PeriphCLKConfig(&clk_init);
  HAL_PWREx_EnableVddUSB();
  __HAL_RCC_USB_CLK_ENABLE();

  stm32h5::EnableInterrupt<USB_DRD_FS_IRQn, Impl>();

  PCD_HandleTypeDef hpcd;

  hpcd.Instance = USB_DRD_FS;

  hpcd.Init.dev_endpoints            = 8;
  hpcd.Init.speed                    = USBD_FS_SPEED;
  hpcd.Init.phy_itface               = PCD_PHY_EMBEDDED;
  hpcd.Init.Sof_enable               = DISABLE;
  hpcd.Init.low_power_enable         = DISABLE;
  hpcd.Init.lpm_enable               = DISABLE;
  hpcd.Init.battery_charging_enable  = DISABLE;
  hpcd.Init.vbus_sensing_enable      = DISABLE;
  hpcd.Init.bulk_doublebuffer_enable = DISABLE;
  hpcd.Init.iso_singlebuffer_enable  = DISABLE;

  HAL_PCD_Init(&hpcd);
}

}   // namespace stm32h5