"""ResBench eq_comparator: authored from the problem and port declaration."""

import pycircuit as ac


def compute(a: ac.u4, b: ac.u4) -> ac.u1:
    return a == b


@ac.module
def Top(a: ac.u4, b: ac.u4) -> ac.u1:
    return compute(a, b)
