"""ResBench polynomial_2: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_0: ac.u8) -> ac.u24:
    x: ac.u24 = in_0
    x = x | 16776960 if in_0 >= 128 else x
    return x * x * x + 3 * x * x + 3 * x + 1


@ac.module
def Top(in_0: ac.u8) -> ac.u24:
    return compute(in_0)
