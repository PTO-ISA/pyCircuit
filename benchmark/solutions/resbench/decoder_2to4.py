"""ResBench decoder_2to4: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u2) -> ac.u4:
    decoded: ac.u4 = 1
    if in_ == 1:
        decoded = 2
    if in_ == 2:
        decoded = 4
    if in_ == 3:
        decoded = 8
    return decoded


@ac.module
def Top(in_: ac.u2) -> ac.u4:
    return compute(in_)
