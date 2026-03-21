from typing import Optional, Literal, cast

import abc
import struct
import re

from hal2.logging.spec_json import EnumSpecJson, MessageSpecJson, ArgSpecJson, ModuleSpecJson, LogSpecJson


class MessageNotFoundError(Exception):
    def __init__(self, message_id: int):
        super().__init__(f"Unknown message ID {message_id:x}.")


class ModuleNotFoundError(Exception):
    def __init_(self, module_id: int):
        super().__init__(f"Unknown module {module_id:x}.")


class EnumSpec:
    """Enum specification."""

    def __init__(self, data: EnumSpecJson):
        """
        Constructor.

        Args:
            data: JSON representation of the enum spec.
        """

        self.name = data["name"]
        self.underlying_type = data["underlying_type"]
        self._options: dict[int, str] = {opt["value"]: opt["name"] for opt in data["options"]}

    def get_option_name(self, value: int) -> str:
        """
        Gets the name of an enum option given its value.

        Args:
            value: Value to map.

        Returns:
            Option name, or `"<unknown>"` when unknown.
        """

        if value in self._options:
            return self._options[value]

        return "<unknown>"


class ValueFormatter[T](abc.ABC):
    """ABC for a value formatter."""

    @abc.abstractmethod
    def format(self, value: T) -> str:
        """
        Formats a value.

        Args:
            value: Value to format.

        Returns:
            Formatted value.
        """


class EnumFormatter(ValueFormatter[int]):
    def __init__(self, enum_spec: EnumSpec):
        """
        Constructor.

        Args:
            enum_spec: Enum specification.
        """

        super().__init__()

        self._spec = enum_spec

    def format(self, value: int) -> str:
        """
        Formats an enum value.

        Args:
            value: Value to format.

        Returns:
            Formatted value.
        """

        return self._spec.get_option_name(value)


class IntFormatter(ValueFormatter[int]):
    """Formatter for integer values."""

    def __init__(self, fmt: Optional[str]):
        """
        Constructor.

        Args:
            fmt: Format string.
        """

        self._fmt = fmt

        self._base: Literal["d", "b", "x", "X"] = "d"
        self._pad_type: Literal[">", "<", "0"] = "<"
        self._pad_len: int = 0

        if fmt is not None:
            int_fmt_re = re.compile(r"^(?P<pad_type>[<>0]?)(?P<pad_len>\d*?)(?P<base>[bxX]?)$")
            m = int_fmt_re.match(fmt)

            if m is None:
                raise RuntimeError(f"Invalid integer format string '{fmt}'.")

            if (pad_type := m.group("pad_type")) != "":
                self._pad_type = cast(Literal[">", "<", "0"], pad_type)
            if (pad_len := m.group("pad_len")) != "":
                self._pad_len = int(pad_len)
            if (base := m.group("base")) != "":
                self._base = cast(Literal["d", "b", "x", "X"], base)

    def format(self, value: int):
        """
        Formats an integer value.

        Args:
            value: Value to format.

        Returns:
            Formatted value.
        """

        # Convert to proper base
        result = str(value)

        if self._base == "b":
            result = f"{value:b}"
        elif self._base == "x":
            result = f"{value:x}"
        elif self._base == "X":
            result = f"{value:X}"

        # Apply padding
        if len(result) < self._pad_len:
            diff = self._pad_len - len(result)
            if self._pad_type == "<":
                result = result + (" " * diff)
            elif self._pad_type == ">":
                result = (" " * diff) + result
            elif self._pad_type == "0":
                result = ("0" * diff) + result

        return result


class FloatFormatter(ValueFormatter[float]):
    """Formatter for floating point values."""

    def __init__(self, fmt: Optional[str]):
        """
        Constructor.

        Args:
            fmt: Format string.
        """

        self._fmt = fmt

    def format(self, value: float) -> str:
        """
        Formats a floating point value.

        Args:
            value: Value to format.

        Returns:
            Formatted value.
        """

        return f"{value:.3f}"


class MessageSpec:
    """Specification for a log message."""

    def __init__(self, data: MessageSpecJson, enum_types: dict[str, EnumSpec]):
        """
        Constructor.

        Args:
            data: JSON representation of the message specification.
            enum_types: List of enum specifications for the module the message is in.
        """
        self.id = data["id"]

        self._template = _create_format_string(data["message_template"], data["arguments"])
        self._args: list[tuple[str, ValueFormatter]] = [
            (arg["name"], _get_formatter(arg, enum_types)) for arg in data["arguments"]
        ]
        self._struct_spec = "<" + "".join(_get_struct_fmt(arg["type"], enum_types) for arg in data["arguments"])

    def decode(
        self,
        payload: bytes,
        wrap_value: Optional[tuple[str, str]] = None,
    ) -> str:
        """
        Decodes a raw message and formats it.

        Args:
            payload: Message payload to decode
            wrap_value: Strings to wrap dynamic values with, or ``None`` to not wrap.

        Returns:
            Formatted message
        """

        # Handle value prefix and postfix
        if wrap_value is not None:
            val_pre, val_post = wrap_value
        else:
            val_pre, val_post = "", ""

        # Handle messages without payload.
        if len(self._args) == 0:
            if len(payload) > 0:
                raise RuntimeError(
                    f"Log message with ID {self.id} has no arguments, but attempted to decode non-empty payload."
                )

            return self._template

        # Decode arguments and format their value.
        formatted_args = {
            name: f"{val_pre}{formatter.format(value)}{val_post}"
            for ((name, formatter), value) in zip(self._args, struct.unpack(self._struct_spec, payload))
        }

        # Substitute the formatted arguments in the message template.
        return self._template.format_map(formatted_args)


class ModuleSpec:
    """Specification of a logging module."""

    def __init__(self, data: ModuleSpecJson):
        """
        Constructor.

        Args:
            data: JSON representation of module specification.
        """

        self.id = data["id"]
        self.name = data["name"]

        # Create enum specifications.
        self._enums: dict[str, EnumSpec] = {name: EnumSpec(spec) for name, spec in data["enums"].items()}

        # Create message specifications.
        self._messages: dict[int, MessageSpec] = {
            msg_spec["id"]: MessageSpec(msg_spec, self._enums) for msg_spec in data["messages"]
        }

    def decode(
        self,
        msg_id: int,
        payload: bytes,
        wrap_value: Optional[tuple[str, str]] = None,
    ) -> str:
        """
        Decodes a message.

        Args:
            msg_id: Message ID.
            payload: Message payload.
            wrap_value: Strings to wrap dynamic values with, or ``None`` to not wrap.

        Returns:
            Formatted message.
        """
        if msg_id not in self._messages:
            raise MessageNotFoundError(msg_id)

        return self._messages[msg_id].decode(payload, wrap_value)


class LoggingSpec:
    """Specification of a collection of logging modules."""

    def __init__(self, data: LogSpecJson):
        """
        Constructor.

        Args:
            data:
        """
        self._modules = {module["id"]: ModuleSpec(module) for module in data["modules"]}

    def decode(
        self,
        module_id: int,
        msg_id: int,
        payload: bytes,
        wrap_value: Optional[tuple[str, str]] = None,
    ) -> tuple[str, str]:
        """
        Decodes a message payload with a corresponding module and message ID.

        Args:
            module_id: ID of the module the message is defined in.
            msg_id: Message ID.
            payload: Message payload.
            wrap_value: Strings to wrap dynamic values with, or ``None`` to not wrap.

        Returns:
            Tuple containing module name and formatted message.
        """
        if module_id not in self._modules:
            raise ModuleNotFoundError(module_id)

        module = self._modules[module_id]
        return module.name, self._modules[module_id].decode(msg_id, payload, wrap_value)


def _get_formatter(arg_spec: ArgSpecJson, enum_types: dict[str, EnumSpec]) -> ValueFormatter:
    if arg_spec["type"] in ["uint8", "uint16", "uint32", "uint64", "int8", "int16", "int32", "int64"]:
        return IntFormatter(arg_spec["format"])

    if arg_spec["type"] in ["float32", "float64"]:
        return FloatFormatter(arg_spec["format"])

    if arg_spec["type"].startswith("enum:"):
        enum_name = arg_spec["type"][len("enum:") :]

        if enum_name not in enum_types:
            raise RuntimeError(f"Unknown enum type '{enum_name}'")

        return EnumFormatter(enum_types[enum_name])

    raise RuntimeError(f"Unknown / unsupported argument type '{arg_spec['type']}'")


def _get_struct_fmt(type_name: str, enum_types: dict[str, EnumSpec]) -> str:
    primitives = {
        "uint8": "B",
        "uint16": "H",
        "uint32": "I",
        "uint64": "Q",
        "int8": "b",
        "int16": "h",
        "int32": "i",
        "int64": "q",
        "float32": "f",
        "float64": "d",
    }

    if type_name in primitives:
        return primitives[type_name]

    if type_name.startswith("enum:"):
        enum_name = type_name[len("enum:") :]

        if enum_name not in enum_types:
            raise RuntimeError(f"Unknown enum type '{enum_name}'.")

        return _get_struct_fmt(enum_types[enum_name].underlying_type, enum_types)

    raise RuntimeError(f"Unknown / unsupported argument type '{type_name}'.")


def _create_format_string(message: str, args: list[ArgSpecJson]) -> str:
    result = message

    for arg in args:
        if arg["format"] is not None:
            result = result.replace(f"{{{arg['name']}:{arg['format']}}}", f"{{{arg['name']}}}")

    return result
