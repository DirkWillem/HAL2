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
  constexpr RCC_PeriphCLKInitTypeDef clk_init = {
      .PeriphClockSelection = RCC_PERIPHCLK_USB,
      .UsbClockSelection    = RCC_USBCLKSOURCE_HSI48,
  };
  HAL_RCCEx_PeriphCLKConfig(&clk_init);
  HAL_PWREx_EnableVddUSB();
  __HAL_RCC_USB_CLK_ENABLE();

  stm32h5::EnableInterrupt<USB_DRD_FS_IRQn, Impl>();

  PCD_HandleTypeDef hpcd;

  hpcd.Instance = USB_DRD_FS;

  hpcd.Init = {
      .dev_endpoints            = 8,
      .speed                    = USBD_FS_SPEED,
      .phy_itface               = PCD_PHY_EMBEDDED,
      .Sof_enable               = DISABLE,
      .low_power_enable         = DISABLE,
      .lpm_enable               = DISABLE,
      .battery_charging_enable  = DISABLE,
      .vbus_sensing_enable      = DISABLE,
      .bulk_doublebuffer_enable = DISABLE,
      .iso_singlebuffer_enable  = DISABLE,
  };
  HAL_PCD_Init(&hpcd);
}

}   // namespace stm32h5