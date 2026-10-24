# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 5

    @ac.rule(guard=lambda: count < 7)
    def increment() -> None:
        count = count + 1

    @ac.rule(after=(increment,))
    def read() -> ac.u8:
        return count
