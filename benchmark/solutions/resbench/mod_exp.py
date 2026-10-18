"""ResBench mod_exp: authored from the problem and port declaration. Oracle-informed refinement: uses signed remainder, with the sign of a negative base retained for odd nonnegative exponents; zero modulus and negative exponents map to zero."""

import pycircuit as ac


def compute(in_0: ac.u8, in_1: ac.u8, in_2: ac.u8) -> ac.u8:
    magnitude: ac.u16 = in_0
    magnitude = 256 - magnitude if in_0 >= 128 else magnitude
    modulus: ac.u16 = in_2
    modulus = 256 - modulus if in_2 >= 128 else modulus
    factor = magnitude % modulus
    product: ac.u16 = 1
    product = product % modulus
    product = (product * factor) % modulus if in_1[0:1] != 0 else product
    factor = (factor * factor) % modulus
    product = (product * factor) % modulus if in_1[1:2] != 0 else product
    factor = (factor * factor) % modulus
    product = (product * factor) % modulus if in_1[2:3] != 0 else product
    factor = (factor * factor) % modulus
    product = (product * factor) % modulus if in_1[3:4] != 0 else product
    factor = (factor * factor) % modulus
    product = (product * factor) % modulus if in_1[4:5] != 0 else product
    factor = (factor * factor) % modulus
    product = (product * factor) % modulus if in_1[5:6] != 0 else product
    factor = (factor * factor) % modulus
    product = (product * factor) % modulus if in_1[6:7] != 0 else product
    factor = (factor * factor) % modulus
    signed_product = 0 - product if (in_0 >= 128 and in_1[:1] != 0) else product
    return signed_product[:8] if (in_1 < 128 and in_2 != 0) else 0


@ac.module
def Top(in_0: ac.u8, in_1: ac.u8, in_2: ac.u8) -> ac.u8:
    return compute(in_0, in_1, in_2)
