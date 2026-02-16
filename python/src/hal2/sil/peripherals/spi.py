from typing import Callable, Optional

import ctypes

import hal2.sil._lib as sil_lib


class SpiMaster:
    """
    Represents a simulated SPI master in a SIL application
    """

    _mosi_data: bytes
    _ext_miso_size_hint_callback: Optional[Callable[[int], None]]
    _ext_mosi_callback: Optional[Callable[[bytes], None]]

    def __init__(self, lib: sil_lib.SilLib, index: int):
        """
        Constructor.

        Args:
            lib: SIL library isntance
            index: SPI master index
        """

        self._lib = lib
        self._index = index

        # MISO data
        miso_size_hint_callback_t = ctypes.CFUNCTYPE(None, ctypes.c_size_t, use_errno=False, use_last_error=False)
        self._miso_size_hint_callback = miso_size_hint_callback_t(self._miso_size_hint_callback)
        self._ext_miso_size_hint_callback = None

        # MOSI data
        mosi_callback_t = ctypes.CFUNCTYPE(
            None, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, use_errno=False, use_last_error=False
        )
        self._mosi_callback = mosi_callback_t(self._mosi_callback)
        self._mosi_data = b""
        self._ext_mosi_callback = None

        with lib.proxy() as proxy:
            proxy.set_spi_master_miso_size_hint_callback(index, self._miso_size_hint_callback)
            proxy.set_spi_master_mosi_callback(index, self._mosi_callback)

    @property
    def name(self) -> str:
        """Name of the SPI master."""

        with self._lib.proxy() as proxy:
            return proxy.get_spi_master_name(self._index)

    def set_miso_size_hint_callback(self, callback: Callable[[int], None]):
        """
        Registers the callback for when the SPI master wants to receive data that is invoked with the amount of bytes
        the SPI master is about to receive.

        Args:
            callback: Callback to register
        """

        self._ext_miso_size_hint_callback = callback

    def simulate_miso_data(self, data: bytes, timestamp_us: Optional[int] = None):
        """
        Simulates MISO data to the SPI master.

        Args:
            data: MISO data to simulate
            timestamp_us: Timestamp at which to simulate the MISO data. If ``None``, uses the current simulated time
        """

        with self._lib.proxy() as proxy:
            timestamp_us = timestamp_us or proxy.now
            proxy.simulate_spi_master_miso(self._index, timestamp_us, data)

    def set_mosi_callback(self, callback: Callable[[bytes], None]):
        """
        Registers a callback for MOSI data of the SPI master. In this case, MOSI data will not be buffered in the
        SPI master on receive. Rather, the callback is invoked

        Args:
            callback: Callback to register
        """

        self._ext_mosi_callback = callback

    def _miso_size_hint_callback(self, size):
        if self._ext_miso_size_hint_callback is not None:
            self._ext_miso_size_hint_callback(size)

    def _mosi_callback(self, data, size):
        if self._ext_mosi_callback is not None:
            self._ext_mosi_callback(ctypes.string_at(data, size))
        else:
            self._mosi_data += ctypes.string_at(data, size)
