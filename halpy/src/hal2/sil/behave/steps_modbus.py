from typing import Any

import re

from behave import when, then, use_step_matcher

import hal2.modbus.spec as modbus_spec
import hal2.modbus.client as modbus_client

__all__ = ["when_i_write_value_to_holding_register", "then_holding_register_has_value"]

use_step_matcher("re")


@when(
    r"I write value (?P<val>\w+) to holding register (?P<holding_register>[A-Za-z0-9_\-/]+)"
)
def when_i_write_value_to_holding_register(context, val, holding_register):
    client = _get_modbus_client(context)

    reg = _find_register(client.holding_registers, holding_register)
    reg_val = _to_register_value(reg, val)

    client.write_register(reg, reg_val)


@then(r"holding register (?P<holding_register>[A-Za-z0-9_\-/]+) has value (?P<val>\w+)")
def then_holding_register_has_value(context, holding_register, val):
    client = _get_modbus_client(context)

    reg = _find_register(client.holding_registers, holding_register)
    reg_val_check = _to_register_value(reg, val)

    reg_val = client.read_register(reg)

    assert reg_val == reg_val_check, (
        f"Holding register '{holding_register}' was expected to hold value {reg_val_check}, but holds {reg_val}"
    )


def _get_modbus_client(context) -> modbus_client.Client:
    if not hasattr(context, "modbus_client"):
        raise RuntimeError("No MODBUS client was initialized")

    return context.modbus_client


def _upper_camel_case_to_upper_snake_case(name: str) -> str:
    part_re = re.compile(r"(?<!^)[A-Z]")
    return part_re.sub(lambda m: "_" + m.group(0).lower(), name).upper()


def _find_register(
    regs: dict[str, modbus_spec.Register], path: str
) -> modbus_spec.Register:
    reg_path = path.split("/")

    top_level_reg = reg_path.pop(0)
    assert top_level_reg in regs, f"No such register '{top_level_reg}'"
    reg = regs[top_level_reg]

    current_path = [top_level_reg]
    while len(reg_path) > 0:
        el = reg_path.pop(0)
        assert el in reg.children, f"No such register '{'/'.join(current_path + [el])}'"

        reg = reg.children[el]
        current_path.append(el)

    return reg


def _to_register_value(reg: modbus_spec.Register, value: str) -> Any:
    if isinstance(reg.type, modbus_spec.EnumType):
        for e in reg.type.enum:  # noqa
            name = e.name
            if value == e.name or _upper_camel_case_to_upper_snake_case(value) == name:
                return e

        assert False, f"Could not convert '{value}' to member of enum '{reg.type.name}'"

    return None
