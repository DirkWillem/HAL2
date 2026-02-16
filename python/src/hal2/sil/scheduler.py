import hal2.sil._lib as sil_lib


class Scheduler:
    """
    Represents the simulated application scheduler
    """

    def __init__(self, lib: sil_lib.SilLib):
        """
        Constructor

        Args:
            lib: SIL library instance
        """

        self._lib = lib

    @property
    def now(self) -> int:
        """Current simulated time in microseconds."""

        with self._lib.proxy() as proxy:
            return proxy.now

    def simulate_for(self, us: int):
        """
        Simulates the system for a given amount of microseconds

        Args:
            us: Amount of microseconds to simulate for
        """

        with self._lib.proxy() as proxy:
            proxy.simulate_until(proxy.now + us)
