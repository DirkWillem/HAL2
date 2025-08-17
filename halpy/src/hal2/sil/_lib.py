import ctypes

import hal2.sil._proxy as proxy


class SilLib:
    """
    Represents a SIL application shared library. Provides access to a SIL proxy.
    """

    _c: ctypes.CDLL

    def __init__(self, lib_path: str):
        """
        Constructor.

        Args:
            lib_path: SIL library path.
        """

        self._c = ctypes.cdll.LoadLibrary(lib_path)

    def __enter__(self):
        with self.proxy() as p:
            p.app_init()
            p.sched_start()

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        with self.proxy() as p:
            p.sched_shutdown(1000)
            p.app_deinit()

    def proxy(self):
        """
        Returns a SIL proxy.

        Returns:
            SIL proxy.
        """
        return proxy.SilProxy(self._c)
