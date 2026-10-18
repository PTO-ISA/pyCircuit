"""ResBench majority: authored from the problem and port declaration."""

import pycircuit as ac


def compute(a: ac.u1, b: ac.u1, c: ac.u1) -> ac.u1:
    return (a & b) | (a & c) | (b & c)


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1) -> ac.u1:
    return compute(a, b, c)
