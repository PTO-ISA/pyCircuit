"""ResBench relu: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_0: ac.u16) -> ac.u16:
    return 0 if in_0 >= 32768 else in_0


@ac.module
def Top(in_0: ac.u16) -> ac.u16:
    return compute(in_0)
