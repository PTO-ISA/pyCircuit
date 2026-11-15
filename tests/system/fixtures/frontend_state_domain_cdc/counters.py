# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac


@ac.module
def Counter(rst_n: ac.u1, *, start: int) -> ac.u8:
    count: ac.u8 = ac.state(init=start, reset=rst_n, reset_polarity="low")

    @ac.rule
    def advance():
        ac.log("info", "domain count", count)
        count = count + 1

    return count


@ac.module
def CallableCounter(rst_n: ac.u1):
    count: ac.u8 = ac.state(init=0, reset=rst_n, reset_polarity="low")

    @ac.method
    def increment() -> None:
        count = count + 1
