# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 1

    @ac.method
    def increment() -> None:
        count = count + 2

    @ac.method
    def bridge() -> ac.u8:
        return count

    @ac.method(guard=lambda: count <= 5)
    def read() -> ac.u8:
        return count

    @ac.method
    def committed() -> ac.u8:
        return count


@ac.module
def Writer(counter: Counter):  # noqa: N802
    @ac.rule
    def write() -> None:
        counter.increment()


@ac.module
def Reader(counter: Counter) -> ac.u8:  # noqa: N802
    @ac.rule
    def read() -> ac.u8:
        return counter.read()

    return read
