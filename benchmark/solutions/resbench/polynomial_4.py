"""ResBench polynomial_4: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_0: ac.u8) -> ac.u32:
    x: ac.u32 = in_0
    x = x | 4294967040 if in_0 >= 128 else x
    return 3 * (x + 2) * (x + 2)


@ac.module
def Top(in_0: ac.u8) -> ac.u32:
    return compute(in_0)
