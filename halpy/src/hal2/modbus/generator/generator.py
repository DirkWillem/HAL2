import json
import pathlib
import re
import textwrap

import hal2.modbus.generator.json_model as jm


def generate_python_from_json(json_path: pathlib.Path, output_path: pathlib.Path, name: str):
    """
    Generates a Python client for the given MODBUS server specification.

    Args:
        json_path: Path to the JSON specification file
        output_path: Path to the output Python file
        name: Name of the client
    """

    spec: jm.SpecJson = json.loads(json_path.read_text())
    output_path.write_text(_gen_python_spec_str(spec, name))


def _gen_discrete_input_bit_spec(bit: jm.DiscreteInputBitJson | jm.DiscreteInputJson, indent: int = 0) -> str:
    return f"""{" " * indent}spec.Bit(name="{bit["name"]}", address={bit["address"]}, access=spec.BitAccess.READONLY)"""


def _gen_discrete_input_spec(di: jm.DiscreteInputJson, indent: int = 0) -> str:
    if len(di["bits"]) == 0 and di["size"] == 1:
        coils = [f""""{di["name"]}": {_gen_discrete_input_bit_spec(di, indent=indent + 8)}"""]
    else:
        coils = [f""""{di["name"]}": {_gen_discrete_input_bit_spec(spec, indent=indent + 8)}""" for spec in di["bits"]]

    coil_str = f"""spec.Bits(
    name="{di["name"]}",
    start_address={di["start_address"]},
    size={di["size"]},
    access=spec.BitAccess.READONLY,
    bits={{\n{",\n".join(coils)},
    }},
)"""

    return textwrap.indent(coil_str, " " * indent)


def _gen_coil_bit_spec(bit: jm.CoilBitJson | jm.CoilJson, indent: int = 0) -> str:
    return f"""{" " * indent}spec.Bit(name="{bit["name"]}", address={bit["address"]}, access=spec.BitAccess("{bit["access"]}"))"""


def _gen_coil_spec(coil: jm.CoilJson, indent: int = 0) -> str:
    if len(coil["bits"]) == 0 and coil["size"] == 1:
        coils = [f"""{" " * (indent + 8)}"{coil["name"]}": {_gen_coil_bit_spec(coil, indent=indent + 8).lstrip()}"""]
    else:
        coils = [
            f"""{" " * (indent + 8)}"{bit["name"]}": {_gen_coil_bit_spec(bit, indent=indent + 8).lstrip()}"""
            for bit in coil["bits"]
        ]

    coil_str = f"""spec.Bits(
    name="{coil["name"]}",
    start_address={coil["start_address"]},
    size={coil["size"]},
    access=spec.BitAccess("{coil["access"]}"),
    bits={{\n{",\n".join(coils)},
    }},
)"""

    return textwrap.indent(coil_str, " " * indent)


def _upper_camel_case_to_upper_snake_case(name: str) -> str:
    part_re = re.compile(r"(?<!^)[A-Z]")
    return part_re.sub(lambda m: "_" + m.group(0).lower(), name).upper()


SCALAR_TYPES: dict[jm.ScalarTypeName, str] = {
    "int16": "spec.ScalarType.I16",
    "int32": "spec.ScalarType.I32",
    "int64": "spec.ScalarType.I64",
    "uint16": "spec.ScalarType.U16",
    "uint32": "spec.ScalarType.U32",
    "uint64": "spec.ScalarType.U64",
    "float32": "spec.ScalarType.F32",
    "float64": "spec.ScalarType.F64",
}


def _gen_enum_spec(e: jm.EnumJson, indent: int = 0):
    members = []

    for m in e["members"]:
        python_name = _upper_camel_case_to_upper_snake_case(m["name"])
        members.append(f"{' ' * (indent + 4)}{python_name} = {m['value']}")

    result = f"""class {e["name"]}(spec.EnumReflection, enum.IntEnum):
{"\n".join(members)}

    @staticmethod
    def get_underlying_type() -> spec.ScalarType:
        return {SCALAR_TYPES[e["underlying_type"]]}"""
    return textwrap.indent(result, " " * indent)


def _gen_data_type(type: jm.TypeJson):
    if type["type"] in SCALAR_TYPES:
        return SCALAR_TYPES[type["type"]]
    elif type["type"] == "enum":
        n = type["enum_name"]
        return f"""spec.EnumType(name="{n}", enum={n})"""
    elif type["type"] == "array":
        return f"""spec.ArrayType(element_type={_gen_data_type(type["element_type"])}, size={type["size"]})"""


def _gen_python_type(type: jm.TypeJson):
    scalar_types: dict[jm.ScalarTypeName, str] = {
        "int16": "int",
        "int32": "int",
        "int64": "int",
        "uint16": "int",
        "uint32": "int",
        "uint64": "int",
        "float32": "float",
        "float64": "float",
    }

    if type["type"] in scalar_types:
        return scalar_types[type["type"]]
    elif type["type"] == "enum":
        return type["enum_name"]
    elif type["type"] == "array":
        return f"list[{_gen_python_type(type['element_type'])}]"


def _gen_register_spec(reg: jm.RegisterJson, reg_type: str, indent: int = 0) -> str:
    if "children" in reg and len(reg["children"]) > 0:
        children_lst = [
            f"""    "{child["name"]}": {_gen_register_spec(child, reg_type, indent=4).lstrip()}"""
            for child in reg["children"]
        ]
        children = textwrap.indent(f"{{\n{',\n'.join(children_lst)},\n}}", " " * 4).lstrip()
    else:
        children = "None"

    result = f"""spec.Register(
    name="{reg["name"]}",
    start_address={reg["start_address"]},
    size={reg["size"]},
    type={_gen_data_type(reg["type"])},
    python_type={_gen_python_type(reg["type"])},
    children={children},
    register_type={reg_type}
)"""

    return textwrap.indent(result, " " * indent)


def _gen_python_spec_str(spec: jm.SpecJson, name: str) -> str:
    types = [_gen_enum_spec(e) for e in spec["types"]["enums"].values()]

    discrete_inputs = []
    discrete_input_members = []
    discrete_inputs_dict = []
    for di in spec["discrete_inputs"]:
        def_name = _upper_camel_case_to_upper_snake_case(di["name"])
        discrete_inputs.append(f"{def_name} = {_gen_discrete_input_spec(di)}")
        discrete_input_members.append(f"    {di['name']} = {def_name}")
        discrete_inputs_dict.append(f'        "{di["name"]}": {def_name}')

    coils = []
    coil_members = []
    coils_dict = []
    for coil in spec["coils"]:
        def_name = _upper_camel_case_to_upper_snake_case(coil["name"])
        coils.append(f"{def_name} = {_gen_coil_spec(coil)}")
        coil_members.append(f"    {coil['name']} = {def_name}")
        coils_dict.append(f'        "{coil["name"]}": {def_name}')

    input_regs = []
    input_reg_members = []
    input_regs_dict = []
    for reg in spec["input_registers"]:
        def_name = _upper_camel_case_to_upper_snake_case(reg["name"])
        reg_spec = _gen_register_spec(reg, "spec.RegisterType.INPUT_REGISTER")
        input_regs.append(f"{def_name}: spec.Register[{_gen_python_type(reg['type'])}] = {reg_spec}")
        input_reg_members.append(f"    {reg['name']} = {def_name}")
        input_regs_dict.append(f'        "{reg["name"]}": {def_name}')

    holding_regs = []
    holding_reg_members = []
    holding_regs_dict = []
    for reg in spec["holding_registers"]:
        def_name = _upper_camel_case_to_upper_snake_case(reg["name"])
        reg_spec = _gen_register_spec(reg, "spec.RegisterType.HOLDING_REGISTER")
        holding_regs.append(f"{def_name}: spec.Register[{_gen_python_type(reg['type'])}] = {reg_spec}")
        holding_reg_members.append(f"    {reg['name']} = {def_name}")
        holding_regs_dict.append(f'        "{reg["name"]}": {def_name}')

    return f"""import enum

import hal2.modbus.spec as spec
import hal2.modbus.client as client


# Type Definitions
# ----------------
{"\n\n\n".join(types)}

# Discrete Inputs
# ---------------
{"\n".join(discrete_inputs)}

# Coils
# -----
{"\n".join(coils)}

# Input Registers
# ---------------
{"\n".join(input_regs)}

# Holding Registers
# -----------------
{"\n".join(holding_regs)}


# Client
class {name}Client(client.Client):
    # Discrete inputs
    discrete_inputs = {{
{",\n".join(discrete_inputs_dict)}
    }}

{"\n".join(discrete_input_members)}

    # Coils
    coils = {{
{",\n".join(coils_dict)}
    }}

{"\n".join(coil_members)}

    # Input registers
    input_registers = {{
{",\n".join(input_regs_dict)}
    }}

{"\n".join(input_reg_members)}

    # Holding registers
    holding_registers = {{
{",\n".join(holding_regs_dict)}
    }}

{"\n".join(holding_reg_members)}

"""
