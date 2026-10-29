# ruff: noqa: F823, F841, N802 -- Compiler-owned state and Module names.
import pycircuit as ac
from void_test.Owners import AutomaticTicker, Counter, RegisteredTicker


@ac.system
def Top():
    step: ac.u8 = 2
    first = Counter()
    alias = first
    second = Counter()
    RegisteredTicker(step)
    AutomaticTicker(step)

    @ac.autorule
    def advance() -> None:
        updated: ac.u8 = alias.add(step)
        second_updated: ac.u8 = second.add(5)
        original: ac.u8 = first.read()
        aliased: ac.u8 = alias.read()
        separate: ac.u8 = second.read()
        ac.log("info", "counters", original, aliased, separate, updated, second_updated)
        step = step + 1
