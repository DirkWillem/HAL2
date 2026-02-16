module;

#include <cstdint>
#include <span>

export module hal.abstract:i2s;

namespace hal {

/**
 * I2S communication standard setting
 */
export enum class I2sStandard {
  Philips,
  LeftJustified,
  RightJustified,
  PcmShortSynchroFrame,
  PcmLongSynchroFrame,
};

/**
 * I2S data format
 */
export enum class I2sDataFormat {
  Bits16,
  Bits16On32BitFrame,
  Bits24On32BitFrame,
  Bits32
};

/**
 * Operating modes for I2S
 */
export enum class I2sOperatingMode { Poll, Dma, DmaCircular };

/**
 * I2S clock polarity
 */
export enum class I2sClockPolarity { Low, High };

/**
 * @brief Concept describing a I2S receiving master running in an RTOS app.
 *
 * @tparam Impl Implementing type
 */
export template <typename Impl, typename Rtos>
concept RtosRxI2sMaster = requires(Impl& impl) {
  {
    impl.Receive(std::declval<std::span<uint16_t>>(),
                 std::declval<typename Rtos::EventGroup&>(),
                 std::declval<uint32_t>(), std::declval<uint32_t>())
  };
};

}   // namespace hal