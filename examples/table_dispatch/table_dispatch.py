# ruff: noqa: F821, F823, F841, N802 -- Compiler-owned state and Module instances.
"""Distribute eight inputs over four independent accumulators."""

import pycircuit as ac


@ac.module
def Accumulator():
    total: ac.u8 = 0

    @ac.method
    def add(value: ac.u8) -> ac.u8:
        updated = total + value
        total = updated
        return updated

    @ac.method
    def read() -> ac.u8:
        return total


@ac.system
def TableDispatch():
    lanes: ac.table[4, Accumulator]
    selected: ac.u2 = 0
    incoming: ac.u8 = 1

    @ac.rule(guard=lambda: incoming <= 8)
    def dispatch() -> None:
        updated = lanes[selected].add(incoming)
        ac.log("info", "dispatch", selected, incoming, updated)
        selected += 1
        incoming += 1

    dispatch()

    @ac.autorule
    def observe() -> None:
        ac.log(
            "info",
            "totals",
            lanes[0].read(),
            lanes[1].read(),
            lanes[2].read(),
            lanes[3].read(),
        )
