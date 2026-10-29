# ruff: noqa: F823, N803 -- Module state and static formal convention.
import pycircuit as ac


@ac.module
def Counter(*, WIDTH: int = 8):  # noqa: N802
    count: ac.bits[WIDTH] = 0

    @ac.method
    def add(value: ac.bits[WIDTH]) -> ac.bits[WIDTH]:
        next_count = count + value
        count = next_count
        return next_count

    @ac.method
    def read() -> ac.bits[WIDTH]:
        return count
