# ruff: noqa: F821, F823, F841 -- Module bindings and state are compiler-owned.
"""A finite stream fans out atomically while one consumer pauses."""

import pycircuit as ac


@ac.module
def Accumulator():  # noqa: N802
    total: ac.u8 = 0

    @ac.method(guard=lambda phase: (phase < 3) | (phase >= 10))
    def finish(phase: ac.u8) -> None:
        ac.log("info", "finish", total)

    @ac.method
    def add(value: ac.u8, phase: ac.u8) -> ac.u8:
        result = total + value
        total = result
        finish(phase)
        return result

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)


@ac.module
def Split(  # noqa: N802
    source: ac.u8, phase: ac.u8, accumulator: ac.module
) -> tuple[ac.u8, ac.u8]:
    accepted: ac.u8 = 0

    @ac.rule
    def split(value: ac.u8, cycle: ac.u8) -> tuple[ac.u8, ac.u8]:
        total = accumulator.add(value, cycle)
        accepted = accepted + 1
        ac.log("info", "split", value, total)
        return value + 1, total

    left, right = split(source, phase)

    @ac.autorule
    def observe() -> None:
        ac.log("info", "accepted", accepted)

    return left, right


@ac.system
def Top():  # noqa: N802
    accumulator: Accumulator
    sent: ac.u8 = 0
    cycle: ac.u8 = 0

    @ac.rule(guard=lambda: sent < 6)
    def produce() -> ac.u8:
        value: ac.u8 = (
            3
            if sent == 0
            else (
                5
                if sent == 1
                else 2 if sent == 2 else 7 if sent == 3 else 1 if sent == 4 else 4
            )
        )
        sent = sent + 1
        ac.log("info", "send", value)
        return value

    incoming: ac.queue[ac.u8] = ac.queue[ac.u8](depth=3)
    incoming = produce()
    left, right = Split(incoming, cycle, accumulator)

    @ac.rule
    def consume_left(value: ac.u8) -> None:
        ac.log("info", "left", value)

    @ac.rule(guard=lambda: cycle >= 7)
    def consume_right(value: ac.u8) -> None:
        ac.log("info", "right", value)

    consume_left(left)
    consume_right(right)

    @ac.autorule
    def tick() -> None:
        cycle = cycle + 1
        ac.log("info", "progress", cycle, sent)
