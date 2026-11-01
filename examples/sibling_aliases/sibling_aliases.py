# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 2

    @ac.method
    def increment_twice() -> tuple[ac.u8, ac.u8]:
        first = count + 1
        second = first + 1
        count = second
        return first, second

    @ac.method
    def read() -> ac.u8:
        return count


@ac.module
def Stepper(counter: ac.module):  # noqa: N802
    alias = counter

    @ac.rule
    def step_twice() -> None:
        first, second = alias.increment_twice()
        ac.log("info", "alias snapshots", first, second, counter.read())

    step_twice()


@ac.module
def Relay(counter: ac.module):  # noqa: N802
    alias = counter
    Stepper(alias)
