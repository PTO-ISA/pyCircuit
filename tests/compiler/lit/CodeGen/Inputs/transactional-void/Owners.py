# ruff: noqa: F823, F841, N802 -- Compiler-owned state and Module names.
import pycircuit as ac


@ac.module
def Counter() -> None:
    total: ac.u8 = 0

    @ac.method
    def add(amount: ac.u8) -> ac.u8:
        updated = total + amount
        total = updated
        return updated

    @ac.method
    def read() -> ac.u8:
        return total


@ac.module
def RegisteredTicker(step: ac.u8) -> None:
    total: ac.u8 = 0

    @ac.rule
    def advance() -> None:
        ac.log("info", "registered", total)
        total = total + step

    advance()
    return


@ac.module
def AutomaticTicker(step: ac.u8) -> None:
    total: ac.u8 = 0

    @ac.autorule
    def advance() -> None:
        ac.log("info", "automatic", total)
        total = total + step + 1

    return None
