# ruff: noqa: F823, F841, N803 -- hardware owner writes and static formals.
import pycircuit as ac


@ac.module
def Counter(*, W: int = 5, INIT: int = W - 2, BIAS: int = 0):
    count: ac.bits[W] = INIT

    @ac.method
    def advance(delta: ac.bits[W]) -> ac.bits[W]:
        proposed = count + delta + BIAS
        count = proposed
        return proposed

    @ac.method
    def read() -> ac.bits[W]:
        return count

    @ac.method
    def highest(value: ac.bits[W]) -> ac.u1:
        return value[W - 1]


@ac.module
def Required(*, WIDTH: int):
    counter = Counter(W=WIDTH)

    @ac.method
    def advance(delta: ac.bits[WIDTH]) -> ac.bits[WIDTH]:
        proposed: ac.bits[WIDTH] = counter.advance(delta)
        return proposed

    @ac.method
    def read() -> ac.bits[WIDTH]:
        old: ac.bits[WIDTH] = counter.read()
        return old
