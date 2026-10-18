"""ResBench mult_4bit: authored from the problem and port declaration."""

import pycircuit as ac


def compute(a: ac.u4, b: ac.u4) -> ac.u8:
    wide_a: ac.u8 = a
    wide_b: ac.u8 = b
    return wide_a * wide_b


@ac.module
def Top(a: ac.u4, b: ac.u4) -> ac.u8:
    return compute(a, b)
