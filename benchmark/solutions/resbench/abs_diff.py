"""ResBench abs_diff: authored from the problem and port declaration."""

import pycircuit as ac


def compute(a: ac.u8, b: ac.u8) -> ac.u8:
    ordered_a = a ^ 128
    ordered_b = b ^ 128
    return (a - b) if ordered_a >= ordered_b else (b - a)


@ac.module
def Top(a: ac.u8, b: ac.u8) -> ac.u8:
    return compute(a, b)
