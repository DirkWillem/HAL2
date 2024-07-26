#pragma once

#include <optional>

#include <constexpr_tools/chrono_ex.h>

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

}   // namespace detail

template <I2cId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2cTxDma = DmaChannel<Id, I2cDmaRequest::Tx, Prio>;

template <I2cId Id, hal::DmaPriority Prio = hal::DmaPriority::Low>
using I2cRxDma = DmaChannel<Id, I2cDmaRequest::Rx, Prio>;

enum class I2cOperatingMode { Poll, Dma };

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
  friend void ::HAL_I2C_ErrorCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef*);
  friend void ::HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef*);

 public:
  static constexpr auto OperatingMode = OM;
  using Pinout                        = detail::I2cPinoutHelper<Id>::Pinout;
  using TxDmaChannel                  = DmaChannel<Id, I2cDmaRequest::Tx>;
  using RxDmaChannel                  = DmaChannel<Id, I2cDmaRequest::Rx>;

  void HandleEventInterrupt() noexcept {
    HAL_I2C_EV_IRQHandler(&hi2c);   //
  }
  void HandleErrorInterrupt() noexcept { HAL_I2C_ER_IRQHandler(&hi2c); }

  static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }

  /**
   * Reads memory from a slave synchronously
   * @param device_address Slave address
   * @param memory_address Memory address
   * @param dest Destination buffer
   * @param timeout Timeout for reading
   * @param size Size of the data to read. Defaults to the destination buffer
   * size
   */
  void ReadMemoryBlocking(uint16_t             device_address,
                          hal::I2cMemAddr auto memory_address,
                          std::span<std::byte> dest, ct::Duration auto timeout,
                          std::optional<std::size_t> size = {}) noexcept {
    dev_addr = device_address;
    mem_addr = memory_address;

    const auto rx_size = std::min(dest.size(), size.value_or(dest.size()));
    rx_buf             = dest.subspan(0, rx_size);

    HAL_I2C_Mem_Read(
        &hi2c, dev_addr, mem_addr,
        detail::I2cMemAddrSize<decltype(memory_address)>(),
        reinterpret_cast<uint8_t*>(dest.data()), rx_size,
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
  }

  /**
   * Writes memory to a slave synchronously
   * @param device_address Slave address
   * @param memory_address Memory address
   * @param data Data to write
   * @param timeout Write timeout
   */
  void WriteMemoryBlocking(uint16_t             device_address,
                           hal::I2cMemAddr auto memory_address,
                           std::span<std::byte> data,
                           ct::Duration auto    timeout) {
    dev_addr = device_address;
    mem_addr = memory_address;

    HAL_I2C_Mem_Write(
        &hi2c, dev_addr, mem_addr,
        detail::I2cMemAddrSize<decltype(memory_address)>(),
        reinterpret_cast<uint8_t*>(data.data()), data.size(),
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
  }

  /**
   * Writes the bitwise representation of a value to a slave synchronously
   * @tparam T Type of the value to write
   * @param device_address Slave address
   * @param memory_address Memory address
   * @param value Data to write
   * @param timeout Write timeout
   */
  template <typename T>
  void WriteMemoryValueBlocking(uint16_t             device_address,
                                hal::I2cMemAddr auto memory_address,
                                const T& value, ct::Duration auto timeout) {
    std::array<std::byte, sizeof(T)> bytes =
        std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    WriteMemoryBlocking(
        device_address, memory_address, bytes,
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
  }

  /**
   * Reads memory from a slave asynchronously
   * @param device_address Slave address
   * @param memory_address Memory address
   * @param dest Destination buffer
   * @param size Size of the memory to read, defaults to destination buffer size
   */
  void ReadMemory(uint16_t device_address, hal::I2cMemAddr auto memory_address,
                  std::span<std::byte>       dest,
                  std::optional<std::size_t> size = {}) noexcept
    requires(OM == I2cOperatingMode::Dma)
  {
    dev_addr           = device_address;
    mem_addr           = memory_address;
    const auto rx_size = std::min(dest.size(), size.value_or(dest.size()));
    rx_buf             = dest.subspan(0, rx_size);

    HAL_I2C_Mem_Read_DMA(&hi2c, dev_addr, mem_addr,
                         detail::I2cMemAddrSize<decltype(memory_address)>(),
                         reinterpret_cast<uint8_t*>(rx_buf.data()),
                         static_cast<uint16_t>(rx_size));
  }

  /**
   * Writes memory to a slave asynchronously
   * @param device_address Slave address
   * @param memory_address Memory address
   * @param data Value to write
   */
  void WriteMemory(uint16_t device_address, hal::I2cMemAddr auto memory_address,
                   std::span<std::byte> data)
    requires(OM == I2cOperatingMode::Dma)
  {
    dev_addr = device_address;
    mem_addr = memory_address;
    HAL_I2C_Mem_Write_DMA(&hi2c, dev_addr, mem_addr,
                          detail::I2cMemAddrSize<decltype(memory_address)>(),
                          reinterpret_cast<uint8_t*>(data.data()), data.size());
  }

  /**
   * Writes the bitwise representation of a value to a slave asynchronously
   * @tparam T Type of the value to write
   * @param device_address Slave address
   * @param memory_address Memory address
   * @param value Data to write
   */
  template <typename T>
  void WriteMemoryValue(uint16_t             device_address,
                        hal::I2cMemAddr auto memory_address, const T& value)
    requires(OM == I2cOperatingMode::Dma)
  {
    std::array<std::byte, sizeof(T)> bytes =
        std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    WriteMemory(device_address, memory_address, bytes);
  }

 protected:
  explicit I2cImpl(Pinout pinout) noexcept
    requires(OM == I2cOperatingMode::Poll)
  {
    // Set up sda and scl pins
    Pin::InitializeAlternate(
        pinout.sda,
        hal::FindPinAFMapping(I2cSdaPinMappings, Id, pinout.sda)->af,
        pinout.pull_sda, hal::PinMode::OpenDrain);
    Pin::InitializeAlternate(
        pinout.scl,
        hal::FindPinAFMapping(I2cSclPinMappings, Id, pinout.scl)->af,
        pinout.pull_scl, hal::PinMode::OpenDrain);

    // Initialize I2C peripheral
    constexpr auto timingr =
        detail::CalculateI2cTiming(CF.PeripheralClkFreq(Id), S);
    static_assert(timingr.has_value());
    detail::SetupI2c(Id, hi2c, AL, 0x30A0A9FD);
  }

  I2cImpl(hal::Dma auto& dma, Pinout pinout) noexcept
    requires(OM == I2cOperatingMode::Dma)
  {
    // Set up sda and scl pins
    Pin::InitializeAlternate(
        pinout.sda,
        hal::FindPinAFMapping(I2cSdaPinMappings, Id, pinout.sda)->af,
        pinout.pull_sda, hal::PinMode::OpenDrain);
    Pin::InitializeAlternate(
        pinout.scl,
        hal::FindPinAFMapping(I2cSclPinMappings, Id, pinout.scl)->af,
        pinout.pull_scl, hal::PinMode::OpenDrain);

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

  void RxComplete() {
    if constexpr (hal::AsyncI2c<Impl>) {
      static_cast<Impl*>(this)->I2cReceiveCallback(dev_addr, rx_buf);
    }
  }

  void TxComplete() {
    if constexpr (hal::AsyncI2c<Impl>) {
      static_cast<Impl*>(this)->I2cTransmitCallback(dev_addr);
    }
  }

  void MemRxComplete() {
    if constexpr (hal::AsyncI2c<Impl>) {
      static_cast<Impl*>(this)->I2cMemReadCallback(dev_addr, mem_addr, rx_buf);
    }
  }

  void MemTxComplete() {
    if constexpr (hal::AsyncI2c<Impl>) {
      static_cast<Impl*>(this)->I2cMemWriteCallback(dev_addr, mem_addr);
    }
  }

 private:
  I2C_HandleTypeDef hi2c{};

  uint16_t             dev_addr{0};
  uint16_t             mem_addr{0};
  std::span<std::byte> rx_buf{};
};

template <I2cId Id>
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

using I2c1 = I2c<I2cId::I2c1>;
using I2c2 = I2c<I2cId::I2c2>;
using I2c3 = I2c<I2cId::I2c3>;
using I2c4 = I2c<I2cId::I2c4>;

}   // namespace stm32g4
