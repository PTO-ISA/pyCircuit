"""ResBench int_sqrt: authored from the problem and port declaration. Negative inputs map to zero; the problem does not define their square root."""

import pycircuit as ac


def compute(in_0: ac.u16) -> ac.u8:
    root: ac.u16 = 0
    for index in range(8):
        trial = root | (128 >> index)
        root = trial if trial * trial <= in_0 else root
    return 0 if in_0 >= 32768 else root[:8]


@ac.module
def Top(in_0: ac.u16) -> ac.u8:
    return compute(in_0)
