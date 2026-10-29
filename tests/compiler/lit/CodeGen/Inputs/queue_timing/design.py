# ruff: noqa: F823, F841 -- Module state is compiler-owned.
"""Replace a Queue head at empty, middle and full old occupancy."""

import pycircuit as ac


@ac.system
def Top():  # noqa: N802
    phase: ac.u8 = 0
    produced: ac.u8 = 0
    swapped: ac.u8 = 0
    consumed: ac.u8 = 0
    last: ac.u8 = 0
    queue = ac.queue[ac.u8](depth=2)
    replacing = (phase == 1) | (phase == 3) | (phase == 5) | (phase == 7)

    @ac.rule(guard=lambda: (phase == 0) | (phase == 2) | (phase == 4) | (phase == 8))
    def produce() -> ac.u8:
        produced = produced + 1
        value: ac.u8 = 11 if phase == 0 else 22
        return value

    @ac.rule
    def swap(value: ac.u8) -> ac.u8:
        swapped = swapped + 1
        return value + 1

    @ac.rule(guard=lambda: (phase == 0) | (phase == 4) | (phase == 6) | (phase == 9))
    def consume(value: ac.u8) -> None:
        consumed = consumed + 1
        last = value

    if replacing:
        queue = swap(queue)
    else:
        queue = produce()
        consume(queue)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, produced, swapped, consumed, last)
        phase = phase + 1
