# ruff: noqa: F823 -- Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    count: ac.u8 = 0

    @ac.method
    def add(value: ac.u8) -> ac.u8:
        next_count = count + value
        count = next_count
        return next_count

    @ac.method
    def read() -> ac.u8:
        return count
