# ruff: noqa: F823, F841, N802 -- Captured Module state.
"""A joined formal consumes only on reached uses, including unused arguments."""

import pycircuit as ac


@ac.system
def Top():
    phase: ac.u8 = 0
    commits: ac.u8 = 0
    queue = ac.queue[ac.u8](depth=2)

    @ac.rule(guard=lambda: (phase == 0) | (phase == 3))
    def produce() -> ac.u8:
        return phase + 10

    queue = produce()

    @ac.method
    def ignore(value: ac.u8) -> ac.u8:
        return ac.u8(9)

    @ac.rule
    def consume(value: ac.u8) -> None:
        if phase == 0:
            value = ac.u8(7)
        if phase == 1:
            ac.log("info", "ignored", ignore(value))
        if phase == 4:
            ac.log("info", "read", value)
        commits = commits + 1
        ac.log("info", "done", phase)

    consume(queue)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, commits)
        phase = phase + 1
