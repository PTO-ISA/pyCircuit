# ruff: noqa: F821, F823, F841, F842, N802 -- Compiler-owned instances, state and type constraints.
import pycircuit as ac


@ac.module
def Provider():
    count: ac.u8 = 250

    @ac.method
    def peek() -> ac.u8:
        return count

    @ac.method
    def advance() -> ac.u8:
        count = count + 1
        return count

    @ac.autorule
    def observe() -> None:
        ac.log("info", "count", count)
