# ruff: noqa: F823 -- captured persistent state proposals.
import pycircuit as ac


@ac.module
def Counter():
    count: ac.u8 = 7

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def add(delta: ac.u8) -> ac.u8:
        count = count + delta
        return count
