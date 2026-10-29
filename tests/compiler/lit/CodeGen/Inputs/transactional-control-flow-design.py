# ruff: noqa: F823, F841, N802 -- Captured Module state.
"""Module, Rule and Method branches share ordinary locals and early returns."""

import pycircuit as ac


@ac.system
def Top():
    phase: ac.u8 = 0
    committed: ac.u8 = 0
    total: ac.u16 = 0

    base = phase + ac.u8(10)
    snapshot = base
    if phase < 2:
        base = base + 1
        if phase == 0:
            extra = ac.u8(3)
        else:
            extra = ac.u8(5)
    else:
        extra = ac.u8(7)

    @ac.method
    def constant_value() -> ac.u8:
        disabled = (ac.u8(3) + ac.u8(4)) != ac.u8(7)
        if disabled:
            return ac.u8(99)
        else:
            if (ac.u8(6) & ac.u8(2)) == ac.u8(2):
                if ac.u8(ac.u1(1) << ac.u8(1)) == ac.u8(0):
                    return ac.u8(7)
                return ac.u8(98)

    @ac.method
    def adjust(value: ac.u8) -> tuple[ac.u8, ac.u8]:
        adjusted = value + 2
        if phase == 0:
            return adjusted + 1, ac.u8(20)
        if phase == 1:
            adjusted = adjusted + 3
            tag = ac.u8(30)
        else:
            tag = ac.u8(40)
        return adjusted, tag

    @ac.method(guard=lambda: phase == 99)
    def blocked(value: ac.u8) -> ac.u8:
        return value + 100

    @ac.autorule
    def calculate() -> None:
        value = base + extra
        fallback = value
        if phase == 0:
            selected = value + 1
            fallback = value + 2
        elif phase == 3:
            selected = value + 3
        else:
            selected = value + 5
        result, tag = adjust(selected)
        committed = committed + 1
        if phase == 2:
            result = blocked(result)
        total = total + result
        ac.log("info", "branch", snapshot, base, extra, fallback, result, tag)
        if phase == 0:
            return
        ac.log("info", "tail", result)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "constant_return", constant_value())
        ac.log("info", "state", phase, committed, total)
        phase = phase + 1
