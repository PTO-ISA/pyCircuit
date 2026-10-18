"""ResBench gradient_descent: authored from the problem and port declaration. Integer arithmetic modulo 2^32; no fixed-point scale is specified."""

import pycircuit as ac


def compute(in_0: ac.u32, in_1: ac.u32, in_2: ac.u32) -> ac.u32:
    return in_2 - in_0 * in_1


@ac.module
def Top(in_0: ac.u32, in_1: ac.u32, in_2: ac.u32) -> ac.u32:
    return compute(in_0, in_1, in_2)
