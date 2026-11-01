# ruff: noqa: F823, F841, N802, N803 -- Captured state and static parameters.
"""A default-parameter Module is allocated separately for each Table element."""

import pycircuit as ac


@ac.module
def Worker(*, STEP: int = 3):
    total: ac.u8 = 0

    @ac.method
    def add(amount: ac.u8) -> None:
        total += amount + ac.u8(STEP)

    @ac.method
    def read() -> ac.u8:
        return total


@ac.module
def GuardedWorker():
    total: ac.u8 = 0

    @ac.method(guard=lambda phase: phase > 1 and total == 0)
    def add(phase: ac.u8) -> ac.u8:
        updated = total + phase + 1
        total = updated
        ac.log("info", "selected", phase, total)
        return updated

    @ac.method
    def read() -> ac.u8:
        return total


@ac.module
def Gate():
    accepted: ac.u8 = 0

    @ac.method(guard=lambda phase: phase != 2)
    def finish(phase: ac.u8) -> None:
        accepted += 1

    @ac.method
    def read() -> ac.u8:
        return accepted
