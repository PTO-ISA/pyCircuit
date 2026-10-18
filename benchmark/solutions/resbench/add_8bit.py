"""ResBench add_8bit: authored from the problem and port declaration."""

import pycircuit as ac


def compute(a: ac.u8, b: ac.u8) -> ac.u9:
    signed_a: ac.u9 = a
    signed_b: ac.u9 = b
    signed_a = signed_a | 256 if a >= 128 else signed_a
    signed_b = signed_b | 256 if b >= 128 else signed_b
    return signed_a + signed_b


@ac.module
def Top(a: ac.u8, b: ac.u8) -> ac.u9:
    return compute(a, b)
