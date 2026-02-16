from typing import Optional

import dataclasses

import hal2.sil.peripherals.gpio as gpio


@dataclasses.dataclass
class SquareWaveProperties:
    mean_frequency: float
    mean_duty_cycle: float

    full_periods: int

    min_period: float
    max_period: float

    min_duty_cycle: float
    max_duty_cycle: float


def analyze_square_wave(edges: list[gpio.Edge]) -> Optional[SquareWaveProperties]:
    us = 1_000_000

    # Periods is a list of tuples containing (low_duration, high_duration)
    ts: list[tuple[float, float]] = []

    n_periods = (len(edges) - 1) // 2

    # Analyze each period
    for i in range(n_periods):
        # Get timestamps and the center edge, which determines the low and high part of the wave
        t0, _ = edges[i * 2]
        t1, edge_mid = edges[i * 2 + 1]
        t2, _ = edges[i * 2 + 2]

        if edge_mid == gpio.Edge.RISING:
            ts.append(((t1 - t0) / us, (t2 - t1) / us))
        else:
            ts.append(((t2 - t1) / us, (t1 - t0) / us))

    # If no periods were detected, return None
    if not ts:
        return None

    # Calculate duty cycles and periods
    duty_cycles = [t_hi / (t_lo + t_hi) for t_lo, t_hi in ts]
    periods = [t_lo + t_hi for t_lo, t_hi in ts]

    mean_period = sum(periods) / len(periods)

    return SquareWaveProperties(
        mean_frequency=1 / mean_period,
        mean_duty_cycle=sum(duty_cycles) / len(duty_cycles),
        full_periods=len(periods),
        min_period=min(periods),
        max_period=max(periods),
        min_duty_cycle=min(duty_cycles),
        max_duty_cycle=max(duty_cycles),
    )
