from typing import Literal

import dataclasses
import re

import hal2.codegen.stm32_pin_af.pin as pin


def gen_spi_i2s_mapping(pins: list[pin.Pin], mcu_family: str, mcu: str) -> str:
    """
    Generates the pin mapping C++ file for the SPI and I2S peripheral of a STM32 MCU as used by HAL2.

    Args:
        pins: Parsed pin data.
        mcu_family: MCU family name (e.g. ``stm32g0``, ``stm32h5``)
        mcu: MCU name (e.g. ``stm32g4ret6``)

    Returns:
        Contents of the C++ module containing the pin mapping.
    """

    # Get the SPI and I2S pin mappings.
    spi_mappings = _get_spi_pin_mappings(pins)
    i2s_mappings = _get_i2s_pin_mappings(pins)

    # Return template contents.
    return f"""module;

#include <array>

export module hal.{mcu_family}:pin_mapping.spi_i2s;

import hal.abstract;

import :peripherals;
import :pin;

namespace {mcu_family} {{

using SpiPinMapping = hal::AFMapping<PinId, SpiId>;
using I2sPinMapping = hal::AFMapping<PinId, I2sId>;

/** SPI SCK pin mappings */
export {_spi_pin_mappings_tmpl("SpiSckPinMappings", spi_mappings.sck)}

/** SPI NSS pin mappings */
export {_spi_pin_mappings_tmpl("SpiNssPinMappings", spi_mappings.nss)}

/** SPI MISO pin mappings */
export {_spi_pin_mappings_tmpl("SpiMisoPinMappings", spi_mappings.miso)}

/** SPI MOSI pin mappings */
export {_spi_pin_mappings_tmpl("SpiMosiPinMappings", spi_mappings.mosi)}

/** I2S SDI pin mappings */
export {_i2s_pin_mappings_tmpl("I2sSdiPinMappings", i2s_mappings.sdi)}

/** I2S SDO pin mappings */
export {_i2s_pin_mappings_tmpl("I2sSdoPinMappings", i2s_mappings.sdo)}

/** I2S WS pin mappings */
export {_i2s_pin_mappings_tmpl("I2sWsPinMappings", i2s_mappings.ws)}

/** I2S CK pin mappings */
export {_i2s_pin_mappings_tmpl("I2sCkPinMappings", i2s_mappings.ck)}

/** I2S mck pin mappings */
export {_i2s_pin_mappings_tmpl("I2sMckPinMappings", i2s_mappings.mck)}

}}
"""


@dataclasses.dataclass
class _SpiPinMappings:
    sck: list[pin.PinAf]
    nss: list[pin.PinAf]
    miso: list[pin.PinAf]
    mosi: list[pin.PinAf]


@dataclasses.dataclass
class _I2sPinMappings:
    sdi: list[pin.PinAf]
    sdo: list[pin.PinAf]
    ws: list[pin.PinAf]
    ck: list[pin.PinAf]
    mck: list[pin.PinAf]


def _get_spi_pin_mappings(pins: list[pin.Pin]) -> _SpiPinMappings:
    """
    Obtains the SPI pin mappings given a list of PIN alternate functions.

    Args:
        pins: Pins to obtain SPI pin mappings for.

    Returns:
        SPI pin mappings.
    """

    af_re = re.compile(r"^(?P<inst>SPI[0-9]+)_(?P<fn>SCK|NSS|MOSI|MISO)$")
    afs: dict[Literal["SCK", "NSS", "MOSI", "MISO"], list[pin.PinAf]] = {"SCK": [], "NSS": [], "MOSI": [], "MISO": []}

    for p in pins:
        for af_name, af_num in p.afs.items():
            if (m := af_re.match(af_name)) is None:
                continue

            afs[m.group("fn")].append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))

    return _SpiPinMappings(sck=afs["SCK"], nss=afs["NSS"], miso=afs["MISO"], mosi=afs["MOSI"])


def _get_i2s_pin_mappings(pins: list[pin.Pin]) -> _I2sPinMappings:
    """
    Obtains the I2S pin mappings given a list of PIN alternate functions.

    Args:
        pins: Pins to obtain I2S pin mappings for.

    Returns:
        I2S pin mappings.
    """

    af_re = re.compile(r"^(?P<inst>I2S[0-9]+)_(?P<fn>SDI|SDO|WS|CK|MCK)$")
    afs: dict[Literal["SDI", "SDO", "WS", "CK", "MCK"], list[pin.PinAf]] = {
        "SDI": [],
        "SDO": [],
        "WS": [],
        "CK": [],
        "MCK": [],
    }

    for p in pins:
        for af_name, af_num in p.afs.items():
            if (m := af_re.match(af_name)) is None:
                continue

            afs[m.group("fn")].append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))

    return _I2sPinMappings(sdi=afs["SDI"], sdo=afs["SDO"], ws=afs["WS"], ck=afs["CK"], mck=afs["MCK"])


def _spi_pin_mappings_tmpl(var_name: str, pins: list[pin.PinAf]) -> str:
    def _line(pin: pin.PinAf) -> str:
        return f"""{{.pin=PinId::Make("{pin.port}", {pin.num}), .peripheral=SpiIdFromName("{pin.instance}"), .af={pin.af}}},"""

    lines = "\n".join([_line(p) for p in pins])

    return f"""inline constexpr std::array<SpiPinMapping, {len(pins)}> {var_name}{{{{
{lines}
}}}};"""


def _i2s_pin_mappings_tmpl(var_name: str, pins: list[pin.PinAf]) -> str:
    def _line(pin: pin.PinAf) -> str:
        return f"""{{.pin=PinId::Make("{pin.port}", {pin.num}), .peripheral=I2sIdFromName("{pin.instance}"), .af={pin.af}}},"""

    lines = "\n".join([_line(p) for p in pins])

    return f"""inline constexpr std::array<I2sPinMapping , {len(pins)}> {var_name}{{{{
{lines}
}}}};"""
