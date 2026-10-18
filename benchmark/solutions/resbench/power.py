"""ResBench power: authored from the problem and port declaration. Negative exponents map to zero because the output is integral; 0^0 maps to one."""

import pycircuit as ac


def compute(in_0: ac.u8, in_1: ac.u4) -> ac.u16:
    factor: ac.u16 = in_0
    factor = factor | 65280 if in_0 >= 128 else factor
    product: ac.u16 = 1
    product = product * factor if in_1[0:1] != 0 else product
    factor = factor * factor
    product = product * factor if in_1[1:2] != 0 else product
    factor = factor * factor
    product = product * factor if in_1[2:3] != 0 else product
    factor = factor * factor
    return 0 if in_1 >= 8 else product


@ac.module
def Top(in_0: ac.u8, in_1: ac.u4) -> ac.u16:
    return compute(in_0, in_1)
