from typing import Optional, Callable

import ctypes


def _func_ptr(ptr, argtypes: list, restype):
    ptr.argtypes = argtypes
    ptr.restype = restype
    return ptr


class SilProxy:
    """
    Provides a proxy to a SIL library, which ensures exception handling during calls through
    a context manager.
    """

    _c: ctypes.CDLL

    _exception: Optional[Exception]

    _parent: Optional["SilProxy"]

    _active_proxy: Optional["SilProxy"] = None

    def __init__(self, dll: ctypes.CDLL, parent: Optional["SilProxy"] = None):
        """
        Constructor.

        Args:
            dll: SIL shared library.
        """

        self._c = dll
        self._exception = None
        self._parent = parent

        # General functions
        self._app_init = _func_ptr(self._c.App_Init, [], None)
        self._app_deinit = _func_ptr(self._c.App_DeInit, [], None)

        # Scheduler-related functions
        self._sched_start = _func_ptr(self._c.Sched_Start, [], None)
        self._sched_shutdown = _func_ptr(self._c.Sched_Shutdown, [ctypes.c_size_t], None)

        self._sched_run_until = _func_ptr(self._c.Sched_RunUntil, [ctypes.c_uint64], None)
        self._sched_run_until_next_time_point = _func_ptr(
            self._c.Sched_RunUntilNextTimePoint, [ctypes.c_uint64], ctypes.c_bool
        )
        self._sched_now = _func_ptr(self._c.Sched_Now, [], ctypes.c_uint64)

        # Error handling-related functions
        self._set_err_callback = _func_ptr(self._c.SetErrorCallback, [ctypes.c_void_p], None)
        self._clear_err_callback = _func_ptr(self._c.ClearErrorCallback, [], None)

        callback_t = ctypes.CFUNCTYPE(None, ctypes.c_char_p, use_errno=False, use_last_error=False)
        self._err_callback = callback_t(self._err_callback)

        # GPIO-related functions
        self._gpio_get_count = _func_ptr(self._c.Gpio_GetGpioCount, [], ctypes.c_size_t)
        self._gpio_get_name = _func_ptr(self._c.Gpio_GetGpioName, [ctypes.c_size_t], ctypes.c_char_p)
        self._gpio_set_input_pin_state = _func_ptr(
            self._c.Gpio_SetInputPinState, [ctypes.c_size_t, ctypes.c_bool], None
        )
        self._gpio_get_output_pin_state = _func_ptr(self._c.Gpio_GetOutputPinState, [ctypes.c_size_t], ctypes.c_bool)
        self._gpio_set_output_pin_edge_callback = _func_ptr(
            self._c.Gpio_SetOutputPinEdgeCallback, [ctypes.c_size_t, ctypes.c_void_p], None
        )
        self._gpio_clear_output_pin_edge_callback = _func_ptr(
            self._c.Gpio_ClearOutputPinEdgeCallback, [ctypes.c_size_t], None
        )

        # UART-related functions
        self._uart_get_count = _func_ptr(self._c.Uart_GetUartCount, [], ctypes.c_size_t)
        self._uart_get_name = _func_ptr(self._c.Uart_GetUartName, [ctypes.c_size_t], ctypes.c_char_p)
        self._uart_simulate_receive = _func_ptr(
            self._c.Uart_SimulateReceive,
            [ctypes.c_size_t, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t],
            ctypes.c_bool,
        )
        self._uart_set_tx_callback = _func_ptr(
            self._c.Uart_SetTransmitCallback, [ctypes.c_size_t, ctypes.c_void_p], ctypes.c_bool
        )

        # SPI-related functions
        self._spi_get_master_count = _func_ptr(self._c.Spi_GetSpiMasterCount, [], ctypes.c_size_t)
        self._spi_get_master_name = _func_ptr(self._c.Spi_GetSpiMasterName, [ctypes.c_size_t], ctypes.c_char_p)
        self._spi_simulate_master_miso = _func_ptr(
            self._c.Spi_SimulateSpiMasterMiso,
            [ctypes.c_size_t, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t],
            ctypes.c_bool,
        )
        self._spi_set_master_miso_size_hint_callback = _func_ptr(
            self._c.Spi_SetSpiMasterMisoSizeHintCallback, [ctypes.c_size_t, ctypes.c_void_p], ctypes.c_bool
        )
        self._spi_clear_master_miso_size_hint_callback = _func_ptr(
            self._c.Spi_ClearSpiMasterMisoSizeHintCallback, [ctypes.c_size_t], ctypes.c_bool
        )
        self._spi_set_master_mosi_callback = _func_ptr(
            self._c.Spi_SetSpiMasterMosiCallback, [ctypes.c_size_t, ctypes.c_void_p], ctypes.c_bool
        )
        self._spi_clear_master_mosi_callback = _func_ptr(
            self._c.Spi_ClearSpiMasterMosiCallback, [ctypes.c_size_t], ctypes.c_bool
        )

    def _activate(self):
        self._set_err_callback(self._err_callback)
        self._parent = SilProxy._active_proxy
        SilProxy._active_proxy = self

    def _check_errors(self):
        self._clear_err_callback()

        if self._exception is not None:
            raise self._exception

    def __enter__(self):
        self._activate()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._check_errors()

        if self._parent is not None:
            self._parent._activate()  # noqa

        SilProxy._active_proxy = self._parent

        return None

    def app_init(self):
        """Initializes the SIL application."""
        self._app_init()

    def app_deinit(self):
        """Deinitializes the SIL application."""
        self._app_deinit()

    def sched_start(self):
        """Starts the scheduler."""
        self._sched_start()

    def sched_shutdown(self, timeout_us: int):
        """Shuts down the scheduler."""
        self._sched_shutdown(timeout_us)

    def simulate_until(self, timestamp_us: int):
        """
        Simulates the system until the given timestamp.

        Args:
            timestamp_us: Time in microseconds until which to simulate.
        """

        self._sched_run_until(timestamp_us)

    def simulate_until_next_time_point(self, upper_bound_us: int) -> bool:
        """
        Simulates the system until the next time point, with an upper bound.

        Args:
            upper_bound_us: Upper bound until which the system may be simulated.

        Returns:
            Whether any events were simulated.
        """

        return self._sched_run_until_next_time_point(upper_bound_us)

    @property
    def now(self):
        """Current simulated time."""

        return self._sched_now()

    @property
    def gpio_count(self) -> int:
        """Number of GPIOs in the simulated system."""

        return self._gpio_get_count()

    def get_gpio_name(self, index: int) -> str:
        """
        Returns the name of a GPIO

        Args:
            index: Index of the GPIO to get the name for

        Returns:
            GPIO name
        """

        return self._gpio_get_name(index).decode("utf-8")

    def set_gpio_input_pin_state(self, index: int, state: bool):
        """
        Sets the state of a GPIO input pin.

        Args:
            index: Index of the GPIO to set the state for
            state: New state of the pin
        """

        self._gpio_set_input_pin_state(index, state)

    def get_gpio_output_pin_state(self, index: int) -> bool:
        """
        Gets the state of a GPIO output pin.

        Args:
            index: Index of the GPIO to get the state for

        Returns:
            Current state of the pin
        """

        return self._gpio_get_output_pin_state(index)

    def set_gpio_output_pin_edge_callback(self, index: int, callback: Callable):
        """
        Sets the edge callback for a GPIO output pin.

        Args:
            index: Index of the GPIO to set the callback for
            callback: Callback to be called on pin state changes
        """

        self._gpio_set_output_pin_edge_callback(index, callback)

    def clear_gpio_output_pin_edge_callback(self, index: int):
        """
        Clears the edge callback for a GPIO output pin.

        Args:
            index: Index of the GPIO to clear the callback for
        """

        self._gpio_clear_output_pin_edge_callback(index)

    @property
    def uart_count(self) -> int:
        """Number of UARTs in the simulated system."""

        return self._uart_get_count()

    def get_uart_name(self, index: int) -> str:
        """
        Returns the name of the UART at the specified index.

        Args:
            index: UART index.

        Returns:
            UART name.
        """

        return self._uart_get_name(index).decode("utf-8")

    def simulate_uart_receive(self, index: int, timestamp_us: int, data: bytes) -> bool:
        """
        Simulates a receival of data of the simulated UART.

        Args:
            index: UART index to simulate a receive for.
            timestamp_us: Timestamp at which to simulate the receive.
            data: Data that should be received.

        Returns:
            Whether the receive was successful.
        """

        buf = ctypes.create_string_buffer(data)
        buf_u8 = ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint8))

        return self._uart_simulate_receive(index, timestamp_us, buf_u8, len(data))

    def set_uart_tx_callback(self, index: int, callback):
        """
        Sets the callback for when a simulated UART transmits data.

        Args:
            index: UART index for which to register the callback.
            callback: Callback to set.
        """

        self._uart_set_tx_callback(index, callback)

    @property
    def spi_master_count(self) -> int:
        """Number of SPI masters in the simulated system."""

        return self._spi_get_master_count()

    def get_spi_master_name(self, index: int) -> str:
        """
        Returns the name of the SPI master at the specified index.

        Args:
            index: SPI master index.

        Returns:
            SPI master name.
        """

        return self._spi_get_master_name(index).decode("utf-8")

    def simulate_spi_master_miso(self, index: int, timestamp_us: int, data: bytes) -> bool:
        """
        Simulates MISO data for a simulated SPI master.

        Args:
            index: SPI master index to simulate MISO for.
            timestamp_us: Timestamp at which to simulate the MISO data.
            data: MISO data to simulate.

        Returns:
            Whether the simulation was successful.
        """

        buf = ctypes.create_string_buffer(data)
        buf_u8 = ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint8))

        return self._spi_simulate_master_miso(index, timestamp_us, buf_u8, len(data))
    
    def set_spi_master_miso_size_hint_callback(self, index: int, callback) -> bool:
        """
        Sets the callback for providing MISO size hints for a simulated SPI master.

        Args:
            index: SPI master index for which to register the callback.
            callback: Callback to set.

        Returns:
            Whether setting the callback was successful.
        """

        return self._spi_set_master_miso_size_hint_callback(index, callback)


    def clear_spi_master_miso_size_hint_callback(self, index: int) -> bool:
        """
        Clears the MISO size hint callback for a simulated SPI master.

        Args:
            index: SPI master index for which to clear the callback.

        Returns:
            Whether clearing the callback was successful.
        """

        return self._spi_clear_master_miso_size_hint_callback(index)

    def set_spi_master_mosi_callback(self, index: int, callback) -> bool:
        """
        Sets the callback for when a simulated SPI master transmits MOSI data.

        Args:
            index: SPI master index for which to register the callback.
            callback: Callback to set.

        Returns:
            Whether setting the callback was successful.
        """

        return self._spi_set_master_mosi_callback(index, callback)

    def clear_spi_master_mosi_callback(self, index: int) -> bool:
        """
        Clears the MOSI callback for a simulated SPI master.

        Args:
            index: SPI master index for which to clear the callback.

        Returns:
            Whether clearing the callback was successful.
        """

        return self._spi_clear_master_mosi_callback(index)

    def _err_callback(self, data):
        err_msg = ctypes.string_at(data).decode("utf-8")
        self._exception = Exception(err_msg)
