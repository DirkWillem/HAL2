from typing import Literal, cast

import dataclasses
import re

import hal2.codegen.stm32_pin_af.pin as pin


def gen_i2c_mapping(pins: list[pin.Pin], mcu_family: str, mcu: str) -> str:
    """
    Generates the pin mapping C++ file for the I2C peripheral of a STM32 MCU as used by HAL2.

    Args:
        pins: Parsed pin data.
        mcu_family: MCU family name (e.g. ``stm32g0``, ``stm32h5``)
        mcu: MCU name (e.g. ``stm32g4ret6``)

    Returns:
        Contents of the C++ module containing the pin mapping.
    """

    # Get the I2C pin mappings.
    i2c_mappings = _get_i2c_pin_mappings(pins)

    # Return template contents.
    return f"""module;

#include <array>

export module hal.{mcu_family}:pin_mapping.i2c;

import hal.abstract;

import :peripherals;
import :pin;

namespace {mcu_family} {{

using I2cPinMapping = hal::AFMapping<PinId, I2cId>;

/** @brief I2C SCL pin mappings. */
export {_i2c_pin_mappings_tmpl("I2cSclPinMappings", i2c_mappings.scl)}

/** @brief I2C SDA pin mappings. */
export {_i2c_pin_mappings_tmpl("I2cSdaPinMappings", i2c_mappings.sda)}

}}
"""


@dataclasses.dataclass
class _I2cPinMappings:
    sda: list[pin.PinAf]
    scl: list[pin.PinAf]


type _I2cAfName = Literal["SDA", "SCL"]


def _get_i2c_pin_mappings(pins: list[pin.Pin]) -> _I2cPinMappings:
    """
    Obtains the I2C pin mappings given a list of pin alternate functions.

    Args:
        pins: Pins to get the I2C pin mappings of.

    Returns:
        I2C pin mappings.
    """

    af_re = re.compile(r"^(?P<inst>I2C[0-9]+)_(?P<fn>SDA|SCL)$")
    afs: dict[_I2cAfName, list[pin.PinAf]] = {"SDA": [], "SCL": []}

    for p in pins:
        for af_name, af_num in p.afs.items():
            if (m := af_re.match(af_name)) is None:
                continue

            afs[cast(_I2cAfName, m.group("fn"))].append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))

    return _I2cPinMappings(scl=afs["SCL"], sda=afs["SDA"])


def _i2c_pin_mappings_tmpl(var_name: str, pins: list[pin.PinAf]) -> str:
    def _line(pin: pin.PinAf) -> str:
        return f"""{{.pin=PinId::Make("{pin.port}", {pin.num}), .peripheral=I2cIdFromName("{pin.instance}"), .af={pin.af}}},"""

    lines = "\n".join([_line(p) for p in pins])

    return f"""inline constexpr std::array<I2cPinMapping, {len(pins)}> {var_name}{{{{
{lines}
}}}};"""
