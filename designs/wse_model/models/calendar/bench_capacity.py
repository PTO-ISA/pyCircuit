# ruff: noqa: N802, F823, F841 -- captured System state.
"""Isolated capacity assertion; this does not execute ReceiveEngine."""

import pycircuit as ac
from wse_calendar.receive_checks import ReceiveChecks


@ac.module
def CapacityProbe(available: bool) -> ac.u8:
    checks = ReceiveChecks()
    accepted: ac.u8 = 0

    @ac.rule
    def step() -> ac.u8:
        checks.require_identity_capacity(available)
        accepted = accepted + 1
        return accepted

    return step


@ac.system
def CapacityAvailableSystem():
    phase: ac.u3 = 0
    accepted = CapacityProbe(True)

    @ac.rule
    def check() -> None:
        expected: ac.u8 = phase
        assert accepted == expected + 1, "capacity permits state advance"
        ac.log("info", "capacity_checked", phase, accepted)
        phase = phase + 1


@ac.system
def CapacityExhaustedSystem():
    phase: ac.u3 = 0
    accepted = CapacityProbe(phase < 2)

    # ndf: verifies=WSEMIG-RX-CAPACITY-VER-001
    @ac.rule
    def check() -> None:
        expected: ac.u8 = phase
        assert accepted == expected + 1, "capacity permits state advance"
        ac.log("info", "capacity_checked", phase, accepted)
        phase = phase + 1
