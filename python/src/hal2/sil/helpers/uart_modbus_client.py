import pymodbus as mb
import pymodbus.client as mb_client
import pymodbus.transport as mb_transport


import hal2.sil.peripherals.uart as uart


class UartModbusClient(mb_client.ModbusBaseSyncClient):
    """
    MODBUS Client implementation compatible with synchronous pymodbus clients that interacts
    over a simulated SIL UART.
    """

    def __init__(
        self,
        u: uart.Uart,
        framer: mb.FramerType = mb.FramerType.RTU,
        retries: int = 0,
        timeout: float = 1.0,
    ):
        """
        Constructor

        Args:
            u: UART instance to communicate over
            framer: MODBUS framer
            retries: Number of retries that should be used
            timeout: Timeout for a MODBUS request
        """

        super().__init__(
            framer=framer,
            retries=retries,
            comm_params=mb_transport.CommParams(comm_name="UART_SIL"),
            trace_packet=None,
            trace_pdu=None,
            trace_connect=None,
        )

        self._uart = u
        self._timeout = timeout

    def connect(self) -> bool:
        """Dummy connect implementation."""

        return True

    def close(self):
        """Dummy close implementation."""

        pass

    def send(self, request: bytes, addr: tuple | None = None) -> int:
        """
        Sends data to the simulated UART

        Args:
            request: Request data to send
            addr: Address (unused)

        Returns:
            Number of bytes sent
        """

        _ = (addr,)

        self._uart.transmit(request)
        return len(request)

    def recv(self, size: int | None) -> bytes:
        """
        Receives data from the simulated UART

        Args:
            size: Maximum number of bytes to receive

        Returns:
            Received data
        """

        result = self._uart.receive(self._timeout)
        if size is not None:
            result = result[:size]
        return result
