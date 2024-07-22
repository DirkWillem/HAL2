#pragma once

#include <stm32g4/internal/i2c_timing.h>
#include <stm32g4/mappings/i2c_pin_mapping.h>

#include <hal/i2c.h>
#include <hal/peripheral.h>

#include <stm32g4/clocks.h>
#include <stm32g4/dma.h>
#include <stm32g4/pin.h>

namespace stm32g4 {

namespace detail {

template <I2cId Id>
struct I2cPinoutHelper {
  struct Pinout {
    consteval Pinout(PinId sda, PinId scl,
                     hal::PinPull pull_sda = hal::PinPull::NoPull,
                     hal::PinPull pull_scl = hal::PinPull::NoPull) noexcept
        : sda{sda}
        , scl{scl}
        , pull_scl{pull_scl}
        , pull_sda{pull_sda} {
      assert(("SDA pin must be valid",
              hal::FindPinAFMapping(I2cSdaPinMappings, Id, sda).has_value()));
      assert(("SCL pin must be valid",
              hal::FindPinAFMapping(I2cSclPinMappings, Id, scl).has_value()));
    }

    PinId sda;
    PinId scl;

    hal::PinPull pull_sda;
    hal::PinPull pull_scl;
  };
};

void SetupI2c(I2cId id, I2C_HandleTypeDef& hi2c,
              hal::I2cAddressLength addr_length, uint32_t timingr) noexcept;

void EnableI2cInterrupts(I2cId id) noexcept;

}   // namespace detail

template <I2cId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2cTxDma = DmaChannel<Id, I2cDmaRequest::Tx, Prio>;

template <I2cId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2cRxDma = DmaChannel<Id, I2cDmaRequest::Rx, Prio>;

enum class I2cOperatingMode { Poll, Dma };

enum class I2cMemoryAddrSize {
  Bit8  = I2C_MEMADD_SIZE_8BIT,
  Bit16 = I2C_MEMADD_SIZE_16BIT
};

template <typename Impl, I2cId Id, auto CF,
          hal::I2cAddressLength AL = hal::I2cAddressLength::Bits7,
          I2cOperatingMode      OM = I2cOperatingMode::Poll,
          hal::I2cSpeedMode     S  = hal::I2cSpeedMode::Standard>
  requires ClockFrequencies<decltype(CF)>
/**
 * Implementation for I2C
 * @tparam Impl I2C implementation class
 * @tparam Id I2C ID
 * @tparam CF Clock Frequencies type to use when initializing
 * @tparam AL I2C Address Length
 * @tparam OM I2C Operating Mode
 * @tparam S I2C Speed
 */
class I2cImpl : public hal::UsedPeripheral {
  friend void ::HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c);

 public:
  static constexpr auto OperatingMode = OM;
  using Pinout                        = detail::I2cPinoutHelper<Id>::Pinout;
  using TxDmaChannel                  = DmaChannel<Id, I2cDmaRequest::Tx>;
  using RxDmaChannel                  = DmaChannel<Id, I2cDmaRequest::Rx>;

  void HandleEventInterrupt() noexcept { HAL_I2C_EV_IRQHandler(&hi2c); }
  void HandleErrorInterrupt() noexcept { HAL_I2C_ER_IRQHandler(&hi2c); }

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  void ReadMemory(uint16_t dev_addr, uint32_t mem_addr,
                  I2cMemoryAddrSize mem_addr_size, std::span<std::byte> dest,
                  std::size_t size, uint32_t timeout) noexcept
    requires(OM == I2cOperatingMode::Poll)
  {
    HAL_I2C_Mem_Read(
        &hi2c, dev_addr, mem_addr, static_cast<uint16_t>(mem_addr_size),
        reinterpret_cast<uint8_t*>(dest.data()),
        static_cast<uint16_t>(std::min(dest.size(), size)), timeout);
  }

  void ReadMemory(uint16_t dev_addr, uint32_t mem_addr,
                  I2cMemoryAddrSize mem_addr_size, std::span<std::byte> dest,
                  std::size_t size, uint32_t timeout) noexcept
    requires(OM == I2cOperatingMode::Dma)
  {
    HAL_I2C_Mem_Read_DMA(&hi2c, dev_addr, mem_addr,
                         static_cast<uint16_t>(mem_addr_size),
                         reinterpret_cast<uint8_t*>(dest.data()),
                         static_cast<uint16_t>(std::min(dest.size(), size)));
  }

 protected:
  I2cImpl(Pinout pinout) noexcept
    requires(OM == I2cOperatingMode::Poll)
  {
    // Set up sda and scl pins
    Pin::InitializeAlternate(
        pinout.sda,
        hal::FindPinAFMapping(I2cSdaPinMappings, Id, pinout.sda)->af,
        pinout.pull_sda);
    Pin::InitializeAlternate(
        pinout.scl,
        hal::FindPinAFMapping(I2cSclPinMappings, Id, pinout.scl)->af,
        pinout.pull_scl);

    // Initialize I2C peripheral
    constexpr auto timingr =
        detail::CalculateI2cTiming(CF.PeripheralClkFreq(Id), S);
    static_assert(timingr.has_value());
    detail::SetupI2c(Id, hi2c, AL, *timingr);
  }

  I2cImpl(hal::Dma auto& dma, Pinout pinout) noexcept
    requires(OM == I2cOperatingMode::Dma)
  {
    // Set up sda and scl pins
    Pin::InitializeAlternate(
        pinout.sda,
        hal::FindPinAFMapping(I2cSdaPinMappings, Id, pinout.sda)->af,
        pinout.pull_sda);
    Pin::InitializeAlternate(
        pinout.scl,
        hal::FindPinAFMapping(I2cSclPinMappings, Id, pinout.scl)->af,
        pinout.pull_scl);

    // Initialize I2C peripheral
    constexpr auto timingr =
        detail::CalculateI2cTiming(CF.PeripheralClkFreq(Id), S);
    static_assert(timingr.has_value());
    detail::SetupI2c(Id, hi2c, AL, *timingr);

    // Set up DMA
    auto& htxdma = dma.template SetupChannel<TxDmaChannel>(
        hal::DmaDirection::MemToPeriph, hal::DmaMode::Normal,
        hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&hi2c, hdmatx, htxdma);

    auto& hrxdma = dma.template SetupChannel<RxDmaChannel>(
        hal::DmaDirection::PeriphToMem, hal::DmaMode::Normal,
        hal::DmaDataWidth::Byte, false, hal::DmaDataWidth::Byte, true);
    __HAL_LINKDMA(&hi2c, hdmarx, hrxdma);

    // Set up interrupts
    detail::EnableI2cInterrupts(Id);
  }

  void Error() {
    if constexpr (hal::AsyncI2c<Impl>) {
      static_cast<Impl*>(this)->I2cErrorCallback();
    }
  }

 private:
  I2C_HandleTypeDef hi2c;
};

template <I2cId Id>
class I2c : public hal::UnusedPeripheral<I2c<Id>> {
  friend void ::HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c);

 public:
  constexpr void HandleEventInterrupt() noexcept {}
  constexpr void HandleErrorInterrupt() noexcept {}

 protected:
  constexpr void Error() noexcept {}

  I2C_HandleTypeDef hi2c{};
};

using I2c1 = I2c<I2cId::I2c1>;
using I2c2 = I2c<I2cId::I2c2>;
using I2c3 = I2c<I2cId::I2c3>;
using I2c4 = I2c<I2cId::I2c4>;

}   // namespace stm32g4
