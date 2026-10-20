"""Raw binary32 2**(-code), with code255 selecting canonical quiet NaN.

The encoded value rounds once with RNE; the constructor returns a scalar word.
Arithmetic effects belong to subsequent consumers of that scale word.
"""

import pycircuit as ac


def negative_power(code: ac.u8) -> ac.u32:
    exponent: ac.u8 = 127 - code
    exponent32: ac.u32 = exponent
    normal: ac.u32 = exponent32 << 23
    shift: ac.u8 = 149 - code
    one: ac.u32 = 1
    subnormal: ac.u32 = one << shift
    result: ac.u32 = normal if code < 127 else subnormal
    result = 0 if code > 149 else result
    return 2143289344 if code == 255 else result


@ac.module
def FP32NegativePower(code: ac.u8) -> ac.u32:  # noqa: N802
    return negative_power(code)
