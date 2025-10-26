import dataclasses
import re

import hal2.codegen.stm32_pin_af.pin as pin


@dataclasses.dataclass
class _UartPinMappings:
    tx: list[pin.PinAf]
    rx: list[pin.PinAf]
    rts: list[pin.PinAf]
    cts: list[pin.PinAf]


def gen_uart_mapping(pins: list[pin.Pin], mcu_family: str, mcu: str) -> str:
    # Get the UART pin mappings
    mappings = _get_uart_pin_mappings(pins)

    # Return template contents
    return f"""module;

#include <array>

export module hal.{mcu_family}:pin_mapping.uart;

import hal.abstract;

import :peripherals;
import :pin;

namespace {mcu_family} {{

using UartPinMapping = hal::AFMapping<PinId, UartId>;

/** TX pin mappings */
export {_uart_pin_mappings_tmpl("UartTxPinMappings", mappings.tx)}

/** RX pin mappings */
export {_uart_pin_mappings_tmpl("UartRxPinMappings", mappings.rx)}

/** RTS pin mappings */
export {_uart_pin_mappings_tmpl("UartRtsPinMappings", mappings.rts)}

/** CTS pin mappings */
export {_uart_pin_mappings_tmpl("UartCtsPinMappings", mappings.cts)}

}}
"""


def _get_uart_pin_mappings(pins: list[pin.Pin]) -> _UartPinMappings:
    af_re = re.compile(r"^(?P<inst>(LP)?US?ART[0-9]+)_(?P<fn>(RX)|(TX)|(RTS)|(CTS))$")

    tx: list[pin.PinAf] = []
    rx: list[pin.PinAf] = []
    rts: list[pin.PinAf] = []
    cts: list[pin.PinAf] = []

    for p in pins:
        for af_name, af_num in p.afs.items():
            if (m := af_re.match(af_name)) is None:
                continue

            match fn := m.group("fn"):
                case "RX":
                    rx.append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))
                case "TX":
                    tx.append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))
                case "RTS":
                    rts.append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))
                case "CTS":
                    cts.append(pin.PinAf(p.port, p.num, m.group("inst"), af_num))
                case _:
                    raise RuntimeError(f"Invalid UART alternate function name '{fn}'")

    return _UartPinMappings(tx=tx, rx=rx, rts=rts, cts=cts)


def _uart_pin_mappings_tmpl(var_name: str, pins: list[pin.PinAf]) -> str:
    def _line(pin: pin.PinAf) -> str:
        return f"""{{.pin=PinId::Make("{pin.port}", {pin.num}), .peripheral=UartIdFromName("{pin.instance}"), .af={pin.af}}},"""

    lines = "\n".join([_line(p) for p in pins])

    return f"""inline constexpr std::array<UartPinMapping, {len(pins)}> {var_name}{{{{
{lines}
}}}};"""
