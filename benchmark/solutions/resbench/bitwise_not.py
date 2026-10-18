"""ResBench bitwise_not: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u8) -> ac.u8:
    return ~in_


@ac.module
def Top(in_: ac.u8) -> ac.u8:
    return compute(in_)
