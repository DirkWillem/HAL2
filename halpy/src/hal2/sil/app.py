import hal2.sil._lib as sil_lib

import hal2.sil.peripherals.uart as uart


class PeripheralNotFoundError(Exception):
    """
    Error raised when a peripheral could not be found.
    """

    def __init__(self, peripheral_type: str, name: str):
        """
        Constructor.

        Args:
            peripheral_type: Name of the peripheral type.
            name: Name of the peripheral that was requested.
        """
        super().__init__(f"{peripheral_type} with name '{name}' could not be found.")


class SilApp:
    """
    Represents a simulated SIL application.
    """

    _sil_lib: sil_lib.SilLib

    def __init__(self, lib_path: str):
        """
        Constructor.

        Args:
            lib_path: Path to the SIL library to simulate.
        """

        self._sil_lib = sil_lib.SilLib(lib_path)

    def __enter__(self):
        self._sil_lib.__enter__()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return self._sil_lib.__exit__(exc_type, exc_val, exc_tb)

    def get_uart(self, index_or_name: int | str) -> uart.Uart:
        """
        Obtains a UART by its index or name

        Args:
            index_or_name: UART index or name

        Returns:
            Requested UART
        """

        if isinstance(index_or_name, int):
            index = index_or_name
        else:
            index = None
            with self._sil_lib.proxy() as proxy:
                for i in range(proxy.uart_count):
                    if proxy.get_uart_name(i) == index_or_name:
                        index = i
                        break

            if index is None:
                raise PeripheralNotFoundError("UART", index_or_name)

        return uart.Uart(self._sil_lib, index)

    def simulate_until(self, timestamp_us: int):
        with self._sil_lib.proxy() as proxy:
            proxy.simulate_until(timestamp_us)
