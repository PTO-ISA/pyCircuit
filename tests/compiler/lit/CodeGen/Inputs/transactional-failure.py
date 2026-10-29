# ruff: noqa: F821, F823, F841, F842, N802 -- Compiler-owned instances and state.
"""A failed consumer discards Queue, caller, callee and unrelated Rule effects."""

import pycircuit as ac


@ac.module
def Accumulator():
    total: ac.u8 = 0

    @ac.method
    def accept(value: ac.u8) -> None:
        total = total + 1
        ac.log("info", "attempt", total, value)
        assert total < 2

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)


@ac.system
def Failure():
    accumulator: Accumulator
    sent: ac.u8 = 0
    accepted: ac.u8 = 0
    heartbeat: ac.u8 = 0
    pending = ac.queue[ac.u8](depth=2)

    @ac.rule
    def produce() -> ac.u8:
        value = sent + 1
        sent = value
        return value

    @ac.rule
    def consume(value: ac.u8) -> None:
        accepted = accepted + 1
        accumulator.accept(value)
        ac.log("info", "received", value)

    pending = produce()
    consume(pending)

    @ac.autorule
    def progress() -> None:
        heartbeat = heartbeat + 1
        ac.log("info", "progress", sent, accepted, heartbeat)
