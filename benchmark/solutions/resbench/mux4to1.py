"""ResBench mux4to1: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u4, sel: ac.u2) -> ac.u1:
    low = sel[:1]
    high = sel[1:2]
    bit0 = in_[:1] & ~low & ~high
    bit1 = in_[1:2] & low & ~high
    bit2 = in_[2:3] & ~low & high
    bit3 = in_[3:4] & low & high
    return bit0 | bit1 | bit2 | bit3


@ac.module
def Top(in_: ac.u4, sel: ac.u2) -> ac.u1:
    return compute(in_, sel)
