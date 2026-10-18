"""ResBench fibonacci: authored from the problem and port declaration. Returns F(n), F(0)=0 and F(1)=1, modulo 2^16; negative n maps to zero."""

import pycircuit as ac


def compute(in_0: ac.u8) -> ac.u16:
    previous: ac.u16 = 0
    current: ac.u16 = 1
    for index in range(127):
        next_value = previous + current
        previous = current if index < in_0 else previous
        current = next_value if index < in_0 else current
    return 0 if in_0 >= 128 else previous


@ac.module
def Top(in_0: ac.u8) -> ac.u16:
    return compute(in_0)
