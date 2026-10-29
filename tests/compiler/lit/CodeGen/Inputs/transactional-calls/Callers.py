# ruff: noqa: F823, F841, N802 -- Compiler-owned state and Module names.
import pycircuit as ac


@ac.module
def Callers(shared: ac.module, separate: ac.module):
    phase: ac.u8 = 0
    first: ac.u8 = 0
    second: ac.u8 = 0

    @ac.method
    def active() -> ac.u1:
        return phase < 6

    @ac.method(guard=lambda: phase != 2)
    def permit() -> None:
        pass

    @ac.rule(guard=lambda: active())
    def caller_a() -> None:
        amount: ac.u8 = 2
        step, updated = shared.add(amount)
        first = first + 1
        ac.log("info", "first", amount, step, updated)
        permit()

    @ac.rule(guard=lambda: active())
    def caller_b() -> None:
        amount: ac.u8 = 4
        shared.add(amount)
        second = second + 1
        before: ac.u8 = shared.read()
        ac.log("info", "second", before)

    if (phase & 1) == 0:
        caller_a()
    else:
        caller_b()

    @ac.autorule
    def other_instance() -> None:
        amount: ac.u8 = 1
        separate.update(amount)

    @ac.autorule
    def progress() -> None:
        ac.log("info", "progress", phase, first, second)
        phase = phase + 1
