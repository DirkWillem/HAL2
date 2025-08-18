from typing import Callable

import ctypes
import enum

import hal2.sil._lib as sil_lib


class Edge(enum.IntEnum):
    """Possible edges of a GPO."""

    RISING = 0
    FALLING = 1


class Gpo:
    """
    Represents a General-Purpose Output (GPO) in a SIL application.
    """

    _lib: sil_lib.SilLib
    _index: int

    def __init__(self, lib: sil_lib.SilLib, index: int):
        """
        Constructor

        Args:
            lib: SIL Library instance
            index: GPO index
        """

        self._lib = lib
        self._index = index

        self._edge_callback_t = ctypes.CFUNCTYPE(None, ctypes.c_int, use_errno=False, use_last_error=False)
        self._edge_callback = None

    @property
    def name(self) -> str:
        """Name of the GPO."""

        with self._lib.proxy() as proxy:
            return proxy.get_gpio_name(self._index)

    @property
    def state(self) -> bool:
        """Current state of the GPO."""

        with self._lib.proxy() as proxy:
            return proxy.get_gpio_output_pin_state(self._index)

    def register_edge_callback(self, callback: Callable[[Edge], None]):
        """
        Registers an edge callback with the GPIO pin

        Args:
            callback: Callback to register
        """

        if self._edge_callback is not None:
            raise RuntimeError(f"An edge callback is already registered for GPO {self.name}")

        self._edge_callback = self._edge_callback_t(lambda e: callback(Edge(e)))
        with self._lib.proxy() as proxy:
            proxy.set_gpio_output_pin_edge_callback(self._index, self._edge_callback)

    def clear_edge_callback(self):
        """
        Clears the GPIO edge callback.
        """

        with self._lib.proxy() as proxy:
            proxy.clear_gpio_output_pin_edge_callback(self._index)
