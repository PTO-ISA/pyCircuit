"""ResBench parity_8bit: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u8) -> ac.u1:
    folded = in_ ^ (in_ >> 4)
    folded = folded ^ (folded >> 2)
    folded = folded ^ (folded >> 1)
    return folded[:1]


@ac.module
def Top(in_: ac.u8) -> ac.u1:
    return compute(in_)
