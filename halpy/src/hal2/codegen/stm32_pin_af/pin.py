import dataclasses


@dataclasses.dataclass
class PinAf:
    """Contains a reference to a pin alternate function."""

    port: str
    """Pin port."""
    num: int
    """Pin number."""
    instance: str
    """Peripheral instance."""
    af: int
    """Pin alternate function number."""


@dataclasses.dataclass
class Pin:
    """Contains information on a pin on an STM32."""

    port: str
    """Pin port."""
    num: int
    """Pin number."""
    afs: dict[str, int]
    """Pin alternate functions."""
