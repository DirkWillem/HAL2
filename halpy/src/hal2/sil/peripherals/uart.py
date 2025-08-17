from typing import Optional

import ctypes

import hal2.sil._lib as sil_lib


class Uart:
    """
    Represents a simulated UART in a SIL application.
    """

    _rx_data: bytes

    def __init__(self, lib: sil_lib.SilLib, index: int):
        """
        Constructor.

        Args:
            lib: SIL library instance.
            index: UART index
        """

        self._lib = lib
        self._index = index
        callback_t = ctypes.CFUNCTYPE(
            None, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, use_errno=False, use_last_error=False
        )
        self._callback = callback_t(self._tx_callback)

        with lib.proxy() as proxy:
            proxy.set_uart_tx_callback(index, self._callback)

    @property
    def name(self) -> str:
        """Name of the UART."""

        with self._lib.proxy() as proxy:
            return proxy.get_uart_name(self._index)

    def transmit(self, data: bytes, timestamp_us: Optional[int] = None) -> bool:
        """
        Simulates a data transmission to the simulated UART

        Args:
            data: Data to transmit.
            timestamp_us: Timestamp at which to send the data. If ``None``, uses the current time
        """

        with self._lib.proxy() as proxy:
            if timestamp_us is None:
                timestamp_us = proxy.now

            return proxy.simulate_uart_receive(self._index, timestamp_us, data)

    def receive(self, timeout: float) -> bytes:
        """
        Receives data from the simulated UART.

        Args:
            timeout: Receive timeout in seconds.

        Returns:
            Bytes transmitted by the simulated UART. When no bytes were transmitted, returns an empty byte array.
        """

        self._rx_data = b""

        with self._lib.proxy() as proxy:
            # Calculate the upper bound based on the receive timeout
            upper_bound_us = proxy.now + int(round(timeout * 1_000_000))

            # Keep on simulating events until either data is received, or the timeout is reached.
            while proxy.simulate_until_next_time_point(upper_bound_us):
                if len(self._rx_data) > 0:
                    break

            return self._rx_data

    def _tx_callback(self, data, len):
        self._rx_data += ctypes.string_at(data, len)
