"""ResBench log2_int: authored from the problem and port declaration. Zero maps to zero; the problem specifies negative inputs map to zero."""

import pycircuit as ac


def compute(in_0: ac.u16) -> ac.u8:
    exponent: ac.u8 = 0
    exponent = 1 if in_0 >= 2 else exponent
    exponent = 2 if in_0 >= 4 else exponent
    exponent = 3 if in_0 >= 8 else exponent
    exponent = 4 if in_0 >= 16 else exponent
    exponent = 5 if in_0 >= 32 else exponent
    exponent = 6 if in_0 >= 64 else exponent
    exponent = 7 if in_0 >= 128 else exponent
    exponent = 8 if in_0 >= 256 else exponent
    exponent = 9 if in_0 >= 512 else exponent
    exponent = 10 if in_0 >= 1024 else exponent
    exponent = 11 if in_0 >= 2048 else exponent
    exponent = 12 if in_0 >= 4096 else exponent
    exponent = 13 if in_0 >= 8192 else exponent
    exponent = 14 if in_0 >= 16384 else exponent
    return 0 if in_0 >= 32768 else exponent


@ac.module
def Top(in_0: ac.u16) -> ac.u8:
    return compute(in_0)
