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

    def monitor_edges(self, duration: float):
        duration_us = int(round(duration * 1_000_000))
        with _GpoEdgeMonitor(self._lib, self) as edge_monitor:
            with self._lib.proxy() as proxy:
                proxy.simulate_until(proxy.now + duration_us)

            return edge_monitor.edges

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


class _GpoEdgeMonitor:
    _gpo: Gpo
    _lib: sil_lib.SilLib
    edges: list[tuple[int, Edge]]

    def __init__(self, lib: sil_lib.SilLib, gpo: Gpo):
        self.edges = []
        self._lib = lib
        self._gpo = gpo

    def __enter__(self):
        self._gpo.register_edge_callback(self._edge_callback)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._gpo.clear_edge_callback()

    def _edge_callback(self, edge: Edge):
        with self._lib.proxy() as proxy:
            self.edges.append((proxy.now, edge))
