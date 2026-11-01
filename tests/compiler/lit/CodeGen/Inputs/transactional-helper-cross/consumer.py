# ruff: noqa: F823, F841, N802 -- Captured System state.
import pycircuit as ac
from helper_case.provider import calculate


@ac.system
def Top():
    phase: ac.u8 = 0

    @ac.autorule
    def observe() -> None:
        result, doubled = calculate(ac.u8(252) + phase)
        ac.log("info", "cross", result, doubled)
        phase = phase + 1
