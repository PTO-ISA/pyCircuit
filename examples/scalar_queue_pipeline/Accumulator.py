# ruff: noqa: F823 -- Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Accumulator():  # noqa: N802
    total: ac.bits[8] = 0

    @ac.method
    def add(value: ac.bits[8]) -> ac.bits[8]:
        next_total = total + value
        total = next_total
        return next_total

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)
