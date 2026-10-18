"""ResBench priority_encoder: authored from the problem and port declaration."""

import pycircuit as ac


def compute(in_: ac.u4) -> ac.u2:
    code: ac.u2 = 0
    if in_ >= 2:
        code = 1
    if in_ >= 4:
        code = 2
    if in_ >= 8:
        code = 3
    return code


@ac.module
def Top(in_: ac.u4) -> ac.u2:
    return compute(in_)
