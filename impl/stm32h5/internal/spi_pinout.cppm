module;

#include <concepts>

export module hal.stm32h5:spi.pinout;

import hstd;

import :spi.config;

import :peripherals;
import :pin;
import :pin_mapping.spi_i2s;

namespace stm32h5 {

template <typename F>
consteval bool CheckSpiConfig(SpiSettings<F> settings, hal::SpiMode mode,
                              hal::SpiTransmissionType tt, bool hw_cs) {
  return settings.mode == mode && settings.transmission_type == tt
         && settings.hardware_cs == hw_cs;
}

/**
 * Struct that contains the pin configuration of a SPI instance. Exact pins
 * that are expected is dependent on the SPI configuration.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
struct SpiPinout;

/**
 * SPI master pinout, full duplex, no hardware CS.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::FullDuplex, false))
struct SpiPinout<S> {
  PinId mosi;   //!< MOSI pin
  PinId miso;   //!< MISO pin
  PinId sck;    //!< Clock pin

  hal::PinPull pull_mosi = hal::PinPull::NoPull;   //!< MOSI pin pull
  hal::PinPull pull_miso = hal::PinPull::NoPull;   //!< MISO pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
};

/**
 * SPI master pinout, full duplex, hardware CS.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::FullDuplex, true))
struct SpiPinout<S> {
  PinId mosi;   //!< MOSI pin
  PinId miso;   //!< MISO pin
  PinId sck;    //!< Clock pin
  PinId cs;     //!< Chip Select pin

  hal::PinPull pull_mosi = hal::PinPull::NoPull;   //!< MOSI pin pull
  hal::PinPull pull_miso = hal::PinPull::NoPull;   //!< MISO pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
  hal::PinPull pull_cs   = hal::PinPull::NoPull;   //!< Chip Select pin pull
};

/**
 * SPI master pinout, transmit only, no hardware CS.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::TxOnly, false))
struct SpiPinout<S> {
  PinId mosi;   //!< MOSI pin
  PinId sck;    //!< Clock pin

  hal::PinPull pull_mosi = hal::PinPull::NoPull;   //!< MOSI pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
};

/**
 * SPI master pinout, transmit only, hardware CS.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::TxOnly, true))
struct SpiPinout<S> {
  PinId mosi;   //!< MOSI pin
  PinId sck;    //!< Clock pin
  PinId cs;     //!< Chip Select pin

  hal::PinPull pull_mosi = hal::PinPull::NoPull;   //!< MOSI pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
  hal::PinPull pull_cs   = hal::PinPull::NoPull;   //!< Chip Select pin pull
};

/**
 * SPI master pinout, receive only, no hardware CS.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::RxOnly, false))
struct SpiPinout<S> {
  PinId miso;   //!< MISO pin
  PinId sck;    //!< Clock pin

  hal::PinPull pull_miso = hal::PinPull::NoPull;   //!< MISO pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
};

/**
 * SPI master pinout, transmit only, hardware CS.
 * @tparam S SPI settings
 */
export template <SpiSettings S>
  requires(CheckSpiConfig(S, hal::SpiMode::Master,
                          hal::SpiTransmissionType::RxOnly, true))
struct SpiPinout<S> {
  PinId miso;   //!< MISO pin
  PinId sck;    //!< Clock pin
  PinId cs;     //!< Chip Select pin

  hal::PinPull pull_miso = hal::PinPull::NoPull;   //!< MISO pin pull
  hal::PinPull pull_sck  = hal::PinPull::NoPull;   //!< SCK pin pull
  hal::PinPull pull_cs   = hal::PinPull::NoPull;   //!< Chip Select pin pull
};

/**
 * Concept to check if the given pinout has a SPI MOSI pin.
 */
template <typename PO>
concept MosiPin = requires(PO p) {
  { p.mosi } -> std::convertible_to<PinId>;
  { p.pull_mosi } -> std::convertible_to<hal::PinPull>;
};

/**
 * Concept to check if the given pinout has a SPI MISO pin.
 */
template <typename PO>
concept MisoPin = requires(PO p) {
  { p.miso } -> std::convertible_to<PinId>;
  { p.pull_miso } -> std::convertible_to<hal::PinPull>;
};

/**
 * Concept to check if the given pinout has a SPI SCK pin.
 */
template <typename PO>
concept SckPin = requires(PO p) {
  { p.sck } -> std::convertible_to<PinId>;
  { p.pull_sck } -> std::convertible_to<hal::PinPull>;
};

/**
 * Concept to check if the given pinout has a SPI hardware CS pin.
 */
template <typename PO>
concept CsPin = requires(PO p) {
  { p.cs } -> std::convertible_to<PinId>;
  { p.pull_cs } -> std::convertible_to<hal::PinPull>;
};

export template <SpiId Id, SpiSettings S>
struct SpiPinoutHelper {
  struct Pinout {
    consteval Pinout(SpiPinout<S> pinout)
        : pinout{pinout} {
      // Validate pinout
      if constexpr (MosiPin<SpiPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(SpiMosiPinMappings, Id, pinout.mosi)
                         .has_value(),
                     "MOSI pin must be valid");
      }
      if constexpr (MisoPin<SpiPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(SpiMisoPinMappings, Id, pinout.miso)
                         .has_value(),
                     "MISO pin must be valid");
      }
      if constexpr (SckPin<SpiPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(SpiSckPinMappings, Id, pinout.sck)
                         .has_value(),
                     "SCK pin must be valid");
      }
      if constexpr (CsPin<SpiPinout<S>>) {
        hstd::Assert(
            hal::FindPinAFMapping(SpiNssPinMappings, Id, pinout.cs).has_value(),
            "CS pin must be valid");
      }
    }

    SpiPinout<S> pinout;
  };

  static void SetupPins(const Pinout& p) {
    if constexpr (MosiPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.mosi,
          hal::FindPinAFMapping(SpiMosiPinMappings, Id, p.pinout.mosi)->af,
          p.pinout.pull_mosi);
    }
    if constexpr (MisoPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.miso,
          hal::FindPinAFMapping(SpiMisoPinMappings, Id, p.pinout.miso)->af,
          p.pinout.pull_miso);
    }
    if constexpr (SckPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.sck,
          hal::FindPinAFMapping(SpiSckPinMappings, Id, p.pinout.sck)->af,
          p.pinout.pull_sck);
    }
    if constexpr (CsPin<SpiPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.cs,
          hal::FindPinAFMapping(SpiNssPinMappings, Id, p.pinout.cs)->af,
          p.pinout.pull_cs);
    }
  }
};

}   // namespace stm32h5