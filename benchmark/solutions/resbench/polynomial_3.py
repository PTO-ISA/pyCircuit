"""ResBench polynomial_3: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_0: ac.u8) -> ac.u16:
    x: ac.u16 = in_0
    x = x | 65280 if in_0 >= 128 else x
    return x * x - x - 6


@ac.module
def Top(in_0: ac.u8) -> ac.u16:
    return compute(in_0)
