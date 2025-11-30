module;

#include <concepts>

#include <stm32h5xx_hal.h>

export module hal.stm32h5:i2s.pinout;

import :i2s.config;
import :peripherals;
import :pin;
import :pin_mapping.spi_i2s;

namespace stm32h5 {

export template <I2sSettings>
struct I2sPinout;

/**
 * @brief I2S pinout, duplex transmission, master clock output.
 *
 * @tparam S I2S settings
 */
export template <I2sSettings S>
  requires(S.transmit_mode == I2sTransmitMode::Duplex && S.master_clock_output)
struct I2sPinout<S> {
  PinId        sdi;                               //!< SDI pin.
  PinId        sdo;                               //!< SDO pin.
  PinId        ck;                                //!< CK pin.
  PinId        mck;                               //!< MCK pin.
  PinId        ws;                                //!< WS pin.
  hal::PinPull pull_sdi = hal::PinPull::NoPull;   //!< SDI pin pull.
  hal::PinPull pull_sdo = hal::PinPull::NoPull;   //!< SDO pin pull.
  hal::PinPull pull_ck  = hal::PinPull::NoPull;   //!< CK pin pull.
  hal::PinPull pull_mck = hal::PinPull::NoPull;   //!< MCK pin pull.
  hal::PinPull pull_ws  = hal::PinPull::NoPull;   //!< WS pin pull.
};

/**
 * @brief I2S pinout, duplex transmission, no master clock output.
 *
 * @tparam S I2S settings
 */
export template <I2sSettings S>
  requires(S.transmit_mode == I2sTransmitMode::Duplex && !S.master_clock_output)
struct I2sPinout<S> {
  PinId        sdi;                               //!< SDI pin.
  PinId        sdo;                               //!< SDO pin.
  PinId        ck;                                //!< CK pin.
  PinId        ws;                                //!< WS pin.
  hal::PinPull pull_sdi = hal::PinPull::NoPull;   //!< SDI pin pull.
  hal::PinPull pull_sdo = hal::PinPull::NoPull;   //!< SDO pin pull.
  hal::PinPull pull_ck  = hal::PinPull::NoPull;   //!< CK pin pull.
  hal::PinPull pull_ws  = hal::PinPull::NoPull;   //!< WS pin pull.
};

/**
 * @brief I2S pinout, transmit only, master clock output.
 *
 * @tparam S I2S settings
 */
export template <I2sSettings S>
  requires(S.transmit_mode == I2sTransmitMode::Tx && S.master_clock_output)
struct I2sPinout<S> {
  PinId        sdo;                               //!< SDO pin.
  PinId        ck;                                //!< CK pin.
  PinId        mck;                               //!< MCK pin.
  PinId        ws;                                //!< WS pin.
  hal::PinPull pull_sdo = hal::PinPull::NoPull;   //!< SDO pin pull.
  hal::PinPull pull_ck  = hal::PinPull::NoPull;   //!< CK pin pull.
  hal::PinPull pull_mck = hal::PinPull::NoPull;   //!< MCK pin pull.
  hal::PinPull pull_ws  = hal::PinPull::NoPull;   //!< WS pin pull.
};

/**
 * @brief I2S pinout, transmit only, no master clock output.
 *
 * @tparam S I2S settings
 */
export template <I2sSettings S>
  requires(S.transmit_mode == I2sTransmitMode::Tx && !S.master_clock_output)
struct I2sPinout<S> {
  PinId        sdo;                               //!< SDO pin.
  PinId        ck;                                //!< CK pin.
  PinId        ws;                                //!< WS pin.
  hal::PinPull pull_sdo = hal::PinPull::NoPull;   //!< SDO pin pull.
  hal::PinPull pull_ck  = hal::PinPull::NoPull;   //!< CK pin pull.
  hal::PinPull pull_ws  = hal::PinPull::NoPull;   //!< WS pin pull.
};

/**
 * @brief I2S pinout, receive only, master clock output.
 *
 * @tparam S I2S settings
 */
export template <I2sSettings S>
  requires(S.transmit_mode == I2sTransmitMode::Rx && S.master_clock_output)
struct I2sPinout<S> {
  PinId        sdi;                               //!< SDI pin.
  PinId        ck;                                //!< CK pin.
  PinId        mck;                               //!< MCK pin.
  PinId        ws;                                //!< WS pin.
  hal::PinPull pull_sdi = hal::PinPull::NoPull;   //!< SDI pin pull.
  hal::PinPull pull_ck  = hal::PinPull::NoPull;   //!< CK pin pull.
  hal::PinPull pull_mck = hal::PinPull::NoPull;   //!< MCK pin pull.
  hal::PinPull pull_ws  = hal::PinPull::NoPull;   //!< WS pin pull.
};

/**
 * @brief I2S pinout, receive only no master clock output.
 *
 * @tparam S I2S settings
 */
export template <I2sSettings S>
  requires(S.transmit_mode == I2sTransmitMode::Rx && !S.master_clock_output)
struct I2sPinout<S> {
  PinId        sdi;                               //!< SDI pin.
  PinId        ck;                                //!< CK pin.
  PinId        ws;                                //!< WS pin.
  hal::PinPull pull_sdi = hal::PinPull::NoPull;   //!< SDI pin pull.
  hal::PinPull pull_ck  = hal::PinPull::NoPull;   //!< CK pin pull.
  hal::PinPull pull_ws  = hal::PinPull::NoPull;   //!< WS pin pull.
};

/** @brief Concept to check if a given pinout has the SDI pin */
template <typename PO>
concept SdiPin = requires(PO p) {
  { p.sdi } -> std::convertible_to<PinId>;
  { p.pull_sdi } -> std::convertible_to<hal::PinPull>;
};

/** @brief Concept to check if a given pinout has the SDO pin */
template <typename PO>
concept SdoPin = requires(PO p) {
  { p.sdo } -> std::convertible_to<PinId>;
  { p.pull_sdo } -> std::convertible_to<hal::PinPull>;
};

/** @brief Concept to check if a given pinout has the WS pin */
template <typename PO>
concept WsPin = requires(PO p) {
  { p.ws } -> std::convertible_to<PinId>;
  { p.pull_ws } -> std::convertible_to<hal::PinPull>;
};

/** @brief Concept to check if a given pinout has the CK pin */
template <typename PO>
concept CkPin = requires(PO p) {
  { p.ck } -> std::convertible_to<PinId>;
  { p.pull_ck } -> std::convertible_to<hal::PinPull>;
};

/** @brief Concept to check if a given pinout has the MCK pin */
template <typename PO>
concept MckPin = requires(PO p) {
  { p.mck } -> std::convertible_to<PinId>;
  { p.pull_mck } -> std::convertible_to<hal::PinPull>;
};

export template <I2sId Id, I2sSettings S>
struct I2sPinoutHelper {
  struct Pinout {
    consteval Pinout(I2sPinout<S> pinout)
        : pinout{pinout} {
      // Validate pinout
      if constexpr (SdiPin<I2sPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(I2sSdiPinMappings, Id, pinout.sdi)
                         .has_value(),
                     "SDI pin must be valid");
      }
      if constexpr (SdoPin<I2sPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(I2sSdoPinMappings, Id, pinout.sdo)
                         .has_value(),
                     "SDO pin must be valid");
      }
      if constexpr (WsPin<I2sPinout<S>>) {
        hstd::Assert(
            hal::FindPinAFMapping(I2sWsPinMappings, Id, pinout.ws).has_value(),
            "WS pin must be valid");
      }
      if constexpr (CkPin<I2sPinout<S>>) {
        hstd::Assert(
            hal::FindPinAFMapping(I2sCkPinMappings, Id, pinout.ck).has_value(),
            "CK pin must be valid");
      }
      if constexpr (MckPin<I2sPinout<S>>) {
        hstd::Assert(hal::FindPinAFMapping(I2sMckPinMappings, Id, pinout.mck)
                         .has_value(),
                     "MCK pin must be valid");
      }
    }

    I2sPinout<S> pinout;
  };

  static void SetupPins(const Pinout& p) {
    if constexpr (SdiPin<I2sPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.sdi,
          hal::FindPinAFMapping(I2sSdiPinMappings, Id, p.pinout.sdi)->af,
          p.pinout.pull_sdi);
    }
    if constexpr (SdoPin<I2sPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.sdo,
          hal::FindPinAFMapping(I2sSdoPinMappings, Id, p.pinout.sdo)->af,
          p.pinout.pull_sdo);
    }
    if constexpr (WsPin<I2sPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.ws,
          hal::FindPinAFMapping(I2sWsPinMappings, Id, p.pinout.ws)->af,
          p.pinout.pull_ws);
    }
    if constexpr (CkPin<I2sPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.ck,
          hal::FindPinAFMapping(I2sCkPinMappings, Id, p.pinout.ck)->af,
          p.pinout.pull_ck);
    }
    if constexpr (MckPin<I2sPinout<S>>) {
      Pin::InitializeAlternate(
          p.pinout.mck,
          hal::FindPinAFMapping(I2sMckPinMappings, Id, p.pinout.mck)->af,
          p.pinout.pull_mck);
    }
  }
};

}   // namespace stm32h5