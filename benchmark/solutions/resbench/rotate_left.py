"""ResBench rotate_left: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u8, shift: ac.u3) -> ac.u8:
    stage = ac.concat(in_[:7], in_[7:8]) if shift[:1] != 0 else in_
    stage = ac.concat(stage[:6], stage[6:8]) if shift[1:2] != 0 else stage
    stage = ac.concat(stage[:4], stage[4:8]) if shift[2:3] != 0 else stage
    return stage


@ac.module
def Top(in_: ac.u8, shift: ac.u3) -> ac.u8:
    return compute(in_, shift)
