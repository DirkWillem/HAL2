import re
import hal2.sil.scheduler as sched

from behave import when, then, use_step_matcher

__all__ = ["when_i_wait"]

use_step_matcher("re")

@when(r"(?:I )?wait for (?P<amount>\d+) (?P<unit>seconds?|milliseconds?|microseconds?)")
def when_i_wait(context, amount, unit):
    match unit:
        case "second" | "seconds":
            us = int(amount) * 1_000_000
        case "millisecond" | "milliseconds":
            us = int(amount) * 1_000
        case "microsecond" | "microseconds":
            us = int(amount)
        case _:
            raise RuntimeError(f"Unknown unit {unit}")

    _get_scheduler(context).simulate_for(us)


def _get_scheduler(context) -> sched.Scheduler:
    if not hasattr(context, "scheduler"):
        raise RuntimeError("No Scheduler was initialized")

    return context.scheduler
