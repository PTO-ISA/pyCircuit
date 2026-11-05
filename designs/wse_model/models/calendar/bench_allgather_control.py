# ruff: noqa: N802, F823, F841 -- pyCircuit System state.
"""Same closed loop with immediate or deliberately delayed external acceptance.

Only this stimulus adapter uses cycle deadlines. The controller receives permits
and its own members' feedback, never a scheduled phase or scheduled completion.
"""

import pycircuit as ac
from wse_calendar.allgather_control import CarrierInput, ControlledAllGather


@ac.system
def AllGatherControlSystem():
    cycle: ac.u10 = 0
    dut = ControlledAllGather(
        CarrierInput(cycle=cycle, post_permit=0xFFFFFFFF, landing_permit=0xFFFFFFFF)
    )

    @ac.rule
    def tick() -> None:
        cycle = cycle + 1


@ac.system
def AllGatherDelayedSystem():
    cycle: ac.u10 = 0
    dut = ControlledAllGather(
        CarrierInput(
            cycle=cycle,
            post_permit=0xFFFFFFFF if cycle >= 5 else 0x7FFFFFFF,
            landing_permit=0xBFFFFFFF if cycle >= 261 and cycle < 264 else 0xFFFFFFFF,
        )
    )

    @ac.rule
    def tick() -> None:
        cycle = cycle + 1
