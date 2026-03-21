from typing import TypedDict, Optional


class EnumOptionSpecJson(TypedDict):
    """TypedDict definition of an enum option in a logging spec."""

    name: str
    """Option name."""
    value: int
    """Option value."""


class EnumSpecJson(TypedDict):
    """TypedDict definition of an enum in a logging spec."""

    name: str
    """Enum type name."""
    underlying_type: str
    """Enum underlying type name."""
    options: list[EnumOptionSpecJson]
    """Enum options."""


class ArgSpecJson(TypedDict):
    """TypedDict definition of an argument specification."""

    name: str
    """Argument name."""
    type: str
    """Argument type."""
    format: Optional[str]
    """Argument format specifier."""


class MessageSpecJson(TypedDict):
    """TypedDict definition of a message specification."""

    id: int
    """Message ID."""
    message_template: str
    """Message template string."""
    arguments: list[ArgSpecJson]
    """Message arguments."""


class ModuleSpecJson(TypedDict):
    """TypedDict definition of a module specification."""

    id: int
    """Module ID."""
    name: str
    """Module name."""
    messages: list[MessageSpecJson]
    """Module messages."""
    enums: dict[str, EnumSpecJson]
    """Module enums."""


class LogSpecJson(TypedDict):
    """TypedDict definition of a logging specification."""

    modules: list[ModuleSpecJson]
    """Modules in the logging specification."""
