# ruff: noqa: F821, F823, F841, N802 -- Captured Module state and instances.
"""Pure calls participate in old-Q, guards and whole-transaction admission."""

import pycircuit as ac


def step(value: ac.u8) -> ac.u8:
    return value + 1


def sum_values(left: ac.u8, right: ac.u8) -> ac.u8:
    return left + right


def active(phase: ac.u8) -> ac.u1:
    return phase < 6


def permitted(phase: ac.u8) -> ac.u1:
    if phase == 1:
        return ac.u1(0)
    return ac.u1(1)


@ac.module
def Accumulator():
    total: ac.u8 = 0

    @ac.method(guard=lambda phase: permitted(phase))
    def finish(phase: ac.u8) -> None:
        pass

    @ac.method
    def add(value: ac.u8, phase: ac.u8) -> ac.u8:
        updated = sum_values(total, value)
        total = updated
        ac.log("info", "add", sum_values(total, ac.u8(0)), updated)
        finish(phase)
        return updated

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)


@ac.system
def Top():
    accumulator: Accumulator
    phase: ac.u8 = 0
    accepted: ac.u8 = 0
    amount = step(phase)

    @ac.rule(guard=lambda: active(phase))
    def produce() -> ac.u8:
        result = accumulator.add(sum_values(amount, ac.u8(0)), phase)
        accepted = step(accepted)
        return result

    output = produce()

    @ac.rule(guard=lambda: phase >= 4)
    def consume(value: ac.u8) -> None:
        ac.log("info", "consume", value)

    consume(output)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "state", phase, accepted)
        phase = step(phase)
