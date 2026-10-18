"""ResBench bin_to_gray: authored from the problem and port declaration."""

import pycircuit as ac


def compute(binary: ac.u4) -> ac.u4:
    return binary ^ (binary >> 1)


@ac.module
def Top(binary: ac.u4) -> ac.u4:
    return compute(binary)
