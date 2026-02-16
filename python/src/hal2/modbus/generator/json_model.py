from typing import TypedDict, Literal, NotRequired

type BitAccessJson = Literal["rw"] | Literal["rw0"] | Literal["rw1"]


class DiscreteInputBitJson(TypedDict):
    name: str
    address: int


class DiscreteInputJson(TypedDict):
    name: str
    start_address: int
    size: int
    bits: list[DiscreteInputBitJson]


class CoilBitJson(TypedDict):
    name: str
    address: int
    access: BitAccessJson


class CoilJson(TypedDict):
    name: str
    start_address: int
    size: int
    access: BitAccessJson
    bits: list[CoilBitJson]


type ScalarTypeName = Literal["int16", "int32", "int64", "uint16", "uint32", "uint64", "float32", "float64"]


class EnumMemberJson(TypedDict):
    name: str
    value: str


class EnumJson(TypedDict):
    name: str
    underlying_type: ScalarTypeName
    members: list[EnumMemberJson]


class TypesJson(TypedDict):
    enums: dict[str, EnumJson]


class ScalarTypeJson(TypedDict):
    type: ScalarTypeName


class EnumTypeJson(TypedDict):
    type: Literal["enum"]
    enum_name: str


class ArrayTypeJson(TypedDict):
    type: Literal["array"]
    element_type: "TypeJson"
    size: int


type TypeJson = ScalarTypeJson | EnumTypeJson | ArrayTypeJson


class RegisterJson(TypedDict):
    name: str
    start_address: int
    size: int
    type: TypeJson
    children: NotRequired[list["RegisterJson"]]


class SpecJson(TypedDict):
    discrete_inputs: list[DiscreteInputJson]
    coils: list[CoilJson]

    input_registers: list[RegisterJson]
    holding_registers: list[RegisterJson]

    types: TypesJson
