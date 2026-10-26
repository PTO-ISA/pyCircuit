# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 2

    @ac.method
    def increment() -> ac.u8:
        count = count + 1
        return count

    @ac.method
    def read() -> ac.u8:
        return count


@ac.module
def Stepper(counter: Counter):  # noqa: N802
    alias = counter

    @ac.rule
    def step_twice() -> None:
        first = counter.increment()
        second = alias.increment()
        ac.log("info", "alias snapshots", first, second, counter.read())


@ac.module
def Relay(counter: Counter):  # noqa: N802
    alias = counter
    Stepper(alias)
