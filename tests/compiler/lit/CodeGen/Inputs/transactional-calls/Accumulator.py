# ruff: noqa: F823, F841, N802 -- Compiler-owned state and Module names.
import pycircuit as ac


@ac.module
def Accumulator():
    total: ac.u8 = 0

    @ac.method
    def increment(amount: ac.u8) -> ac.u8:
        amount = amount + 1
        return amount

    @ac.method(guard=lambda: total < 12)
    def add(amount: ac.u8) -> tuple[ac.u8, ac.u8]:
        step = increment(amount)
        updated = update(step)
        return step, updated

    @ac.method
    def update(amount: ac.u8) -> ac.u8:
        updated = total + amount
        total = updated
        return updated

    @ac.method
    def read() -> ac.u8:
        return total

    @ac.autorule
    def observe() -> None:
        ac.log("info", "total", total)
