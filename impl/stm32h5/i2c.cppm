module;

#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <stm32h5xx.h>
#include <stm32h5xx_hal_conf.h>
#include <stm32h5xx_hal_i2c.h>
#include <stm32h5xx_hal_rcc.h>

export module hal.stm32h5:i2c;

import hal.abstract;

import :peripherals;
import :clocks;
import :pin;
import :pin_mapping.i2c;
import :dma;
import :i2c.timing;
import :nvic;

extern "C" {
[[maybe_unused]] void HAL_I2C_ErrorCallback(I2C_HandleTypeDef*);
[[maybe_unused]] void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef*);
[[maybe_unused]] void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef*);
[[maybe_unused]] void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef*);
[[maybe_unused]] void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef*);
}

namespace stm32h5 {

export struct I2cPinout {
  PinId sda;   //!< SDA pin.
  PinId scl;   //!< SCL pin.

  hal::PinPull pull_sda = hal::PinPull::NoPull;   //!< Pull-up/pull-down configuration for \c sda.
  hal::PinPull pull_scl = hal::PinPull::NoPull;   //!< Pull-up/pull-down configuration for \c scl.
};

/**
 * I2C Transmit DMA channel.
 * @tparam Id I2C Id.
 * @tparam Prio DMA Priority.
 */
export template <I2cId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2cTxDma = DmaChannel<Id, I2cDmaRequest::Tx, Prio>;

/**
 * I2C Receive DMA channel.
 * @tparam Id I2C Id.
 * @tparam Prio DMA Priority.
 */
export template <I2cId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2cRxDma = DmaChannel<Id, I2cDmaRequest::Rx, Prio>;

/**
 * @brief Pinout helper for I2C.
 * @tparam Id I2C instance.
 */
template <I2cId Id>
struct I2cPinoutHelper {
  struct Pinout {
    /**
     * @brief Constructor.
     * @param sda SDA pin.
     * @param scl SCL pin.
     * @param pull_sda Pull-up/pull-down configuration for \c sda.
     * @param pull_scl Pull-up/pull-down configuration for \c scl.
     */
    consteval Pinout(I2cPinout pinout) noexcept
        : pinout{pinout} {
      hstd::Assert(hal::FindPinAFMapping(I2cSdaPinMappings, Id, pinout.sda).has_value(),
                   "SDA pin must be valid.");
      hstd::Assert(hal::FindPinAFMapping(I2cSclPinMappings, Id, pinout.scl).has_value(),
                   "SCL pin must be valid.");

      sda_af = hal::FindPinAFMapping(I2cSdaPinMappings, Id, pinout.sda)->af;
      scl_af = hal::FindPinAFMapping(I2cSclPinMappings, Id, pinout.scl)->af;
    }

    /**
     * @brief Configures the pins in the pinout for use with the I2C periperhal.
     */
    void Initialize() const noexcept {
      Pin::InitializeAlternate(pinout.sda, sda_af, pinout.pull_sda, hal::PinMode::OpenDrain);
      Pin::InitializeAlternate(pinout.scl, scl_af, pinout.pull_scl, hal::PinMode::OpenDrain);
    }

    I2cPinout pinout;

    unsigned sda_af{0};   //!< Alternate function number for \c sda.
    unsigned scl_af{0};   //!< Alternate function number for \c scl.
  };
};

/** @brief Supported operating modes for the I2C implementation. */
export enum class I2cOperatingMode : uint8_t {
  DmaRtos,   //!< DMA in an RTOS environment.
};

export enum class I2cSourceClock {
  Default,   //!< Default source clock; PCLK1 for I2C1 and I2C2, PCLK3 for I2C3.
  Pclk1,     //!< Peripheral Clock 1, available for I2C1 and I2C2.
  Pclk3,     //!< Peripheral Clock 3, available for I2C3.
  Pll3R,     //!< PLL3 R output.
  Hsi,       //!< HSI clock.
  Csi,       //!< CSI clock.
};

/** @brief Configuration of an I2C peripheral. */
export struct I2cSettings {
  I2cOperatingMode      operating_mode;                                //!< I2C operating mode.
  I2cSourceClock        source_clock = I2cSourceClock::Default;        //!< I2C source clock.
  hal::I2cSpeedMode     speed_mode   = hal::I2cSpeedMode::Standard;    //!< Speed mode.
  hal::I2cAddressLength addr_len     = hal::I2cAddressLength::Bits7;   //!< Address length.
};

/**
 * @brief Configures the source clock for an I2C peripheral.
 *
 * @tparam Id ID of the I2C peripheral.
 * @tparam CS Clock tree configuration.
 * @tparam SrcClk Source clock to configure.
 * @return Whether configuring the source clock succeeded.
 */
template <I2cId Id, ClockSettings CS, I2cSourceClock SrcClk>
bool SetupI2cSourceClock() noexcept {
  using enum I2cSourceClock;
  using enum I2cId;

  if constexpr (Id == I2c1) {
    hstd::Assert(hstd::IsOneOf<Default, Pclk1, Pll3R, Hsi, Csi>(SrcClk),
                 "I2C1 source clock must be one of PCLK1, PLL3R, HSI or CSI.");

    RCC_PeriphCLKInitTypeDef clk_init{.PeriphClockSelection = RCC_PERIPHCLK_I2C1};

    switch (SrcClk) {
    case Default: [[fallthrough]];
    case Pclk1: clk_init.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1; break;
    case Pll3R: clk_init.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PLL3R; break;
    case Hsi: clk_init.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI; break;
    case Csi: clk_init.I2c1ClockSelection = RCC_I2C1CLKSOURCE_CSI; break;
    default: return false;   // Unreachable.
    }

    return HAL_RCCEx_PeriphCLKConfig(&clk_init) == HAL_OK;
  }

  if constexpr (Id == I2c2) {
    hstd::Assert(hstd::IsOneOf<Default, Pclk1, Pll3R, Hsi, Csi>(SrcClk),
                 "I2C2 source clock must be one of PCLK1, PLL3R, HSI or CSI.");

    RCC_PeriphCLKInitTypeDef clk_init{.PeriphClockSelection = RCC_PERIPHCLK_I2C2};

    switch (SrcClk) {
    case Default: [[fallthrough]];
    case Pclk1: clk_init.I2c1ClockSelection = RCC_I2C2CLKSOURCE_PCLK1; break;
    case Pll3R: clk_init.I2c1ClockSelection = RCC_I2C2CLKSOURCE_PLL3R; break;
    case Hsi: clk_init.I2c1ClockSelection = RCC_I2C2CLKSOURCE_HSI; break;
    case Csi: clk_init.I2c1ClockSelection = RCC_I2C2CLKSOURCE_CSI; break;
    default: return false;   // Unreachable.
    }

    return HAL_RCCEx_PeriphCLKConfig(&clk_init) == HAL_OK;
  }

  if constexpr (Id == I2c3) {
    hstd::Assert(hstd::IsOneOf<Default, Pclk3, Pll3R, Hsi, Csi>(SrcClk),
                 "I2C3 source clock must be one of PCLK3, PLL3R, HSI or CSI.");

    RCC_PeriphCLKInitTypeDef clk_init{.PeriphClockSelection = RCC_PERIPHCLK_I2C3};

    switch (SrcClk) {
    case Default: [[fallthrough]];
    case Pclk3: clk_init.I2c1ClockSelection = RCC_I2C3CLKSOURCE_PCLK3; break;
    case Pll3R: clk_init.I2c1ClockSelection = RCC_I2C3CLKSOURCE_PLL3R; break;
    case Hsi: clk_init.I2c1ClockSelection = RCC_I2C3CLKSOURCE_HSI; break;
    case Csi: clk_init.I2c1ClockSelection = RCC_I2C3CLKSOURCE_CSI; break;
    default: return false;   // Unreachable.
    }

    return HAL_RCCEx_PeriphCLKConfig(&clk_init) == HAL_OK;
  }
}

/**
 * @brief Returns the source clock frequency for a given I2C given the used clock configuration and
 * source clock.
 *
 * @tparam CS Clock Settings.
 * @param source_clock I2C source clock.
 * @param id I2C instance.
 * @return I2C source clock frequency.
 */
template <ClockSettings CS>
consteval auto I2cSourceClockFrequency(I2cSourceClock source_clock, I2cId id) {
  using enum I2cSourceClock;
  using enum I2cId;

  // Handle default source clock.
  if (source_clock == Default) {
    switch (id) {
    case I2c1: [[fallthrough]];
    case I2c2: source_clock = Pclk1; break;
    case I2c3: source_clock = Pclk3; break;
    default: std::unreachable();
    }
  }

  switch (source_clock) {
  case Pclk1:
    return CS.system_clock_settings.Apb1PeripheralsClockFrequency(CS.SysClkSourceClockFrequency());
  case Pclk3:
    return CS.system_clock_settings.Apb3PeripheralsClockFrequency(CS.SysClkSourceClockFrequency());
  case Pll3R: return CS.pll.pll3.OutputR(CS.Pll3SourceClockFrequency());
  case Hsi: return CS.HsiFrequency();
  case Csi: return CS.CsiFrequency();
  default: std::unreachable();
  }
}

/**
 * @brief Enables the clock for the given I2C peripheral.
 *
 * @param id ID of the I2C peripheral.
 */
void EnableI2cClk(I2cId id) {
  switch (id) {
  case I2cId::I2c1: __HAL_RCC_I2C1_CLK_ENABLE(); break;
  case I2cId::I2c2: __HAL_RCC_I2C2_CLK_ENABLE(); break;
  case I2cId::I2c3: __HAL_RCC_I2C3_CLK_ENABLE(); break;
  default: std::unreachable();
  }
}

template <I2cId Id>
constexpr IRQn_Type GetI2cEventIrqn() noexcept {
  switch (Id) {
  case I2cId::I2c1: return I2C1_EV_IRQn;
  case I2cId::I2c2: return I2C2_EV_IRQn;
  case I2cId::I2c3: return I2C3_EV_IRQn;
  default: std::unreachable();
  }
}

template <I2cId Id>
consteval IRQn_Type GetI2cErrIrqn() noexcept {
  switch (Id) {
  case I2cId::I2c1: return I2C1_ER_IRQn;
  case I2cId::I2c2: return I2C2_ER_IRQn;
  case I2cId::I2c3: return I2C3_ER_IRQn;
  default: std::unreachable();
  }
}

template <I2cId Id, typename Impl>
void EnableI2cInterrupts() noexcept {
  EnableInterrupt<GetI2cEventIrqn<Id>(), Impl>();
  EnableInterrupt<GetI2cErrIrqn<Id>(), Impl>();
}

/**
 * @brief Converts a \c I2cAddressLength to a HAL I2C addressing mode.
 *
 * @param addr_length Address length.
 * @return STM32 HAL I2C addressing mode.
 */
[[nodiscard]] constexpr uint32_t ToHalAddressingMode(hal::I2cAddressLength addr_length) noexcept {
  switch (addr_length) {
  case hal::I2cAddressLength::Bits7: return I2C_ADDRESSINGMODE_7BIT;
  case hal::I2cAddressLength::Bits10: return I2C_ADDRESSINGMODE_10BIT;
  default: std::unreachable();
  }
}

/**
 * @brief Returns the I2C memory address size given the address type.
 * @tparam T Memory address type.
 * @return Memory address size.
 */
template <typename T>
[[nodiscard]] consteval uint32_t I2cMemAddrSize() {
  if constexpr (std::is_same_v<std::decay_t<T>, uint8_t>) {
    return I2C_MEMADD_SIZE_8BIT;
  } else if constexpr (std::is_same_v<std::decay_t<T>, uint16_t>) {
    return I2C_MEMADD_SIZE_16BIT;
  } else {
    std::unreachable();
  }
}

template <I2cOperatingMode OM, typename... Ts>
class I2cImplBase {
 protected:
  using EG = int;
};

template <typename OS>
class I2cImplBase<I2cOperatingMode::DmaRtos, OS> {
 protected:
  using EG                            = typename OS::EventGroup;
  static constexpr uint32_t TxDoneBit = (0b1U << 0U);
  static constexpr uint32_t RxDoneBit = (0b1U << 1U);
  static constexpr uint32_t ErrorBit  = (0b1U << 2U);

  EG                        event_group{};
  std::tuple<EG*, uint32_t> active_event_group{};
};

/**
 * @brief Implementation for I2C.
 *
 * @tparam Impl Implementation type.
 * @tparam Id I2C ID.
 * @tparam CS Clock Settings.
 * @tparam IS I2C Settings.
 */
export template <typename Impl, I2cId Id, ClockSettings CS, I2cSettings IS, typename... Rest>
class I2cImpl
    : public hal::UsedPeripheral
    , I2cImplBase<IS.operating_mode, Rest...> {
  friend void ::HAL_I2C_ErrorCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef*);

 public:
  using Pinout       = I2cPinoutHelper<Id>::Pinout;
  using TxDmaChannel = DmaChannel<Id, I2cDmaRequest::Tx>;
  using RxDmaChannel = DmaChannel<Id, I2cDmaRequest::Rx>;

  /**
   * @brief Returns a singleton instance of the I2C instance.
   * @return Singleton instance.
   */
  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  void HandleEventInterrupt() noexcept { HAL_I2C_EV_IRQHandler(&hi2c); }
  void HandleErrorInterrupt() noexcept { HAL_I2C_ER_IRQHandler(&hi2c); }

  /**
   * @brief Reads memory from a device.
   *
   * @param device_address Device address.
   * @param memory_address Memory address.
   * @param dest Destination buffer.
   * @param size Size of the memory to read, or \c std::nullopt if the size of the destination
   * buffer should be used.
   * @return Whether reading succeeded.
   */
  hal::I2cReadResult ReadMemory(uint16_t device_address, hal::I2cMemAddr auto memory_address,
                                std::span<std::byte> dest, hstd::Duration auto timeout,
                                std::optional<std::size_t> size = {}) noexcept
    requires(IS.operating_mode == I2cOperatingMode::DmaRtos)
  {
    constexpr auto RxDoneBit = I2cImplBase<IS.operating_mode, Rest...>::RxDoneBit;
    constexpr auto ErrorBit  = I2cImplBase<IS.operating_mode, Rest...>::ErrorBit;

    // Reset the bits that could be set from the interrupt.
    this->event_group.ClearBits(RxDoneBit | ErrorBit);

    // Read memory.
    if (!ReadMemory(device_address, memory_address, dest, this->event_group, RxDoneBit, size)) {
      return hal::I2cReadResult::FailedToStartRead;
    }

    // Wait for the event group to finish.
    const auto bits = this->event_group.Wait(RxDoneBit | ErrorBit, timeout);

    if (bits.has_value()) {
      if ((*bits & ErrorBit) == ErrorBit) {
        return hal::I2cReadResult::Error;
      } else {
        return hal::I2cReadResult::Ok;
      }
    } else {
      return hal::I2cReadResult::Timeout;
    }
  }

  bool ReadMemory(uint16_t device_address, hal::I2cMemAddr auto memory_address,
                  std::span<std::byte>                                  dest,
                  typename I2cImplBase<IS.operating_mode, Rest...>::EG& event_group,
                  uint32_t bitmask, std::optional<std::size_t> size = {}) noexcept
    requires(IS.operating_mode == I2cOperatingMode::DmaRtos)
  {
    // 7-bit address must be left-shifted by 1 bit.
    device_address <<= 1U;

    this->active_event_group = std::make_pair(&event_group, bitmask);
    const auto rx_size       = std::min(dest.size(), size.value_or(dest.size()));
    const auto mem_size      = I2cMemAddrSize<decltype(memory_address)>();
    const auto read_result   = HAL_I2C_Mem_Read_DMA(
        &hi2c, device_address, static_cast<uint16_t>(memory_address), mem_size,
        reinterpret_cast<uint8_t*>(dest.data()), static_cast<uint16_t>(rx_size));
    if (read_result != HAL_OK) {
      return false;
    }

    __HAL_DMA_DISABLE_IT(hi2c.hdmarx, DMA_IT_HT);
    return true;
  }

  void Error() {
    if constexpr (IS.operating_mode == I2cOperatingMode::DmaRtos) {
      constexpr auto ErrorBit = I2cImplBase<IS.operating_mode, Rest...>::ErrorBit;

      auto [p_eg, bitmask] = this->active_event_group;
      if (p_eg != nullptr) {
        p_eg->SetBitsFromInterrupt(ErrorBit);
      }
    }
  }

  void RxComplete() {
    if constexpr (IS.operating_mode == I2cOperatingMode::DmaRtos) {
      constexpr auto RxDoneBit = I2cImplBase<IS.operating_mode, Rest...>::RxDoneBit;

      auto [p_eg, bitmask] = this->active_event_group;
      if (p_eg != nullptr) {
        p_eg->SetBitsFromInterrupt(RxDoneBit);
      }
    }
  }

  void TxComplete() {
    if constexpr (IS.operating_mode == I2cOperatingMode::DmaRtos) {
      constexpr auto TxDoneBit = I2cImplBase<IS.operating_mode, Rest...>::TxDoneBit;

      auto [p_eg, bitmask] = this->active_event_group;
      if (p_eg != nullptr) {
        p_eg->SetBitsFromInterrupt(TxDoneBit);
      }
    }
  }

  void MemRxComplete() {
    if constexpr (IS.operating_mode == I2cOperatingMode::DmaRtos) {
      constexpr auto RxDoneBit = I2cImplBase<IS.operating_mode, Rest...>::RxDoneBit;

      auto [p_eg, bitmask] = this->active_event_group;
      if (p_eg != nullptr) {
        p_eg->SetBitsFromInterrupt(RxDoneBit);
      }
    }
  }

  void MemTxComplete() {
    if constexpr (IS.operating_mode == I2cOperatingMode::DmaRtos) {
      constexpr auto TxDoneBit = I2cImplBase<IS.operating_mode, Rest...>::TxDoneBit;

      auto [p_eg, bitmask] = this->active_event_group;
      if (p_eg != nullptr) {
        p_eg->SetBitsFromInterrupt(TxDoneBit);
      }
    }
  }

 protected:
  explicit I2cImpl(hal::Dma auto& dma, Pinout pinout) noexcept
    requires(IS.operating_mode == I2cOperatingMode::DmaRtos)
  {
    // Validate that the DMA channels exist.
    using Dma = std::decay_t<decltype(dma)>;
    static_assert(Dma::template ChannelEnabled<TxDmaChannel>(),
                  "When using I2C in DmaRtos mode, the TX DMA channel must be configured.");
    static_assert(Dma::template ChannelEnabled<RxDmaChannel>(),
                  "When using I2C in DmaRtos mode, the RX DMA channel must be configured.");

    // Set up pins
    pinout.Initialize();

    // Initialize the I2C peripheral
    const auto     f_src_clk = SetupI2cSourceClock<Id, CS, IS.source_clock>();
    constexpr auto FSrc      = I2cSourceClockFrequency<CS>(IS.source_clock, Id);
    constexpr auto TimingReg = CalculateI2cTiming(FSrc, IS.speed_mode);
    static_assert(TimingReg.has_value());

    EnableI2cClk(Id);
    hi2c.Instance = GetI2cPointer(Id);
    hi2c.Init     = {
            .Timing           = *TimingReg,
            .OwnAddress1      = 0,
            .AddressingMode   = ToHalAddressingMode(IS.addr_len),
            .DualAddressMode  = I2C_DUALADDRESS_DISABLE,
            .OwnAddress2      = 0,
            .OwnAddress2Masks = I2C_OA2_NOMASK,
            .GeneralCallMode  = I2C_GENERALCALL_DISABLE,
            .NoStretchMode    = I2C_NOSTRETCH_DISABLE,
    };

    HAL_I2C_Init(&hi2c);
    HAL_I2CEx_ConfigAnalogFilter(&hi2c, I2C_ANALOGFILTER_ENABLE);
    HAL_I2CEx_ConfigDigitalFilter(&hi2c, 0);

    // Initialize DMA.
    auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal, hal::DmaDataWidth::Byte, false,
        hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&hi2c, hdmatx, htxdma);

    auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
        hal::DmaDirection::PeriphToMem, hal::DmaMode::Normal, hal::DmaDataWidth::Byte, false,
        hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&hi2c, hdmarx, hrxdma);

    // Enable I2C interrupts.
    EnableI2cInterrupts<Id, Impl>();
  }

 private:
  I2C_HandleTypeDef hi2c{};
};

export template <I2cId Id>
class I2c : public hal::UnusedPeripheral<I2c<Id>> {
  friend void ::HAL_I2C_ErrorCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef*);

 public:
  constexpr void HandleEventInterrupt() noexcept {}
  constexpr void HandleErrorInterrupt() noexcept {}

 protected:
  constexpr void Error() noexcept {}
  constexpr void RxComplete() noexcept {}
  constexpr void TxComplete() noexcept {}
  constexpr void MemRxComplete() noexcept {}
  constexpr void MemTxComplete() noexcept {}

  I2C_HandleTypeDef hi2c{};
};

export using I2c1 = I2c<I2cId::I2c1>;
export using I2c2 = I2c<I2cId::I2c2>;
export using I2c3 = I2c<I2cId::I2c3>;

}   // namespace stm32h5