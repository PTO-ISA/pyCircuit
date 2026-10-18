"""ResBench left_shift: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u8, shift: ac.u3) -> ac.u8:
    return in_ << shift


@ac.module
def Top(in_: ac.u8, shift: ac.u3) -> ac.u8:
    return compute(in_, shift)
