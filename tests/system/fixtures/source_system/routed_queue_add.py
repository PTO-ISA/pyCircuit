# ruff: noqa: F823, F841 -- captured module-owned state updates.
"""Two queued operands commit together and wait for an output consumer."""

import pycircuit as ac


@ac.system
def QueueAdd():
    phase: ac.u8 = 0
    left = ac.queue[ac.u8](depth=1, latency=1)
    right = ac.queue[ac.u8](depth=1, latency=1)
    result = ac.queue[ac.u8](depth=1, latency=1)

    @ac.rule
    def seed_left() -> None:
        if phase == 0:
            left.push(7)

    @ac.rule
    def seed_right() -> None:
        if phase == 2:
            right.push(35)

    @ac.rule
    def add() -> None:
        lhs = left.pop()
        rhs = right.pop()
        result.push(lhs + rhs)

    @ac.rule
    def consume() -> None:
        if phase >= 5:
            value = result.pop()
            ac.log("info", "queue result", phase, value)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
