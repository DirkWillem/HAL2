from typing import Optional, Type

import abc
import enum
import dataclasses


class BitAccess(enum.Enum):
    """Allowed access to a coil."""

    READ_WRITE = "rw"
    READ_WRITE_0 = "rw0"
    READ_WRITE_1 = "rw1"
    READONLY = "ro"


@dataclasses.dataclass
class Bit:
    """Represents a single bit in a MODBUS specification."""

    name: str
    """Bit name."""
    address: int
    """Bit address."""
    access: BitAccess
    """Allowed access to the bit"""


@dataclasses.dataclass
class Bits:
    """Represents one or more bits in a MODBUS server specification."""

    name: str
    """Bits name."""
    start_address: int
    """Address of the first bit."""
    size: int
    """Number of bits"""
    access: BitAccess
    """Access to the bits"""

    bits: dict[str, Bit]
    """Individual bits"""


class ScalarType(enum.Enum):
    """Scalar types supported by MODBUS."""

    U16 = enum.auto()
    """Unsigned 16-bit integer."""
    U32 = enum.auto()
    """Unsigned 32-bit integer."""
    U64 = enum.auto()
    """Unsigned 64-bit integer."""
    I16 = enum.auto()
    """Signed 16-bit integer."""
    I32 = enum.auto()
    """Signed 32-bit integer."""
    I64 = enum.auto()
    """Signed 64-bit integer."""
    F32 = enum.auto()
    """32-bit floating-point number."""
    F64 = enum.auto()
    """64-bit floating-point number."""


class EnumReflection:
    """ABC for enum reflection support"""

    @staticmethod
    @abc.abstractmethod
    def get_underlying_type() -> ScalarType:
        """Returns the underlying type of the enum.

        Returns:
            Enum underlying type.
        """
        ...


@dataclasses.dataclass
class EnumType[T]:
    """Enum type"""

    name: str
    """Enum type."""
    enum: Type[EnumReflection]
    """Type of the enum in Python"""


@dataclasses.dataclass
class ArrayType:
    """Fixed-size array type."""

    element_type: "DataType"
    """Element data type"""
    size: int
    """Number of elements in the array."""


type DataType = ScalarType | EnumType | ArrayType
"""Represents the possible data types of a register"""


class RegisterType(enum.Enum):
    """Possible register types."""

    INPUT_REGISTER = enum.auto()
    """Input register."""
    HOLDING_REGISTER = enum.auto()
    """Holding register."""


@dataclasses.dataclass
class Register[T]:
    """Represents a register in a MODBUS server specification."""

    name: str
    """Register name."""
    start_address: int
    """Register start address."""
    size: int
    """Register size."""
    type: DataType
    """Register data type."""
    python_type: Type[T]
    """Python representation of the register type."""
    children: Optional[dict[str, "Register"]]
    """Child registers."""
    register_type: RegisterType
    """Register type."""
