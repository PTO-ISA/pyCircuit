"""ResBench modulo_op: authored from the problem and port declaration."""

import pycircuit as ac


def compute(a: ac.u8, b: ac.u8) -> ac.u8:
    return a % b


@ac.module
def Top(a: ac.u8, b: ac.u8) -> ac.u8:
    return compute(a, b)
