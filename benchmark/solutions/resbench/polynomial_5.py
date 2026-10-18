"""ResBench polynomial_5: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_0: ac.u8, in_1: ac.u8) -> ac.u16:
    a: ac.u16 = in_0
    b: ac.u16 = in_1
    a = a | 65280 if in_0 >= 128 else a
    b = b | 65280 if in_1 >= 128 else b
    return (a + b) * (a + b) - (a - b) * (a - b)


@ac.module
def Top(in_0: ac.u8, in_1: ac.u8) -> ac.u16:
    return compute(in_0, in_1)
