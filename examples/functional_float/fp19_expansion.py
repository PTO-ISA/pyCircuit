"""Exact finite E8M10-with-sign expansion to binary64.

All exponent codes are finite, including 255. This explicit numerical format
is distinct from IEEE NaN/infinity encoding and has signed zeros/subnormals.
"""

import pycircuit as ac


def expand_finite19(value: ac.u19) -> ac.u64:
    sign: ac.u1 = value[18:19]
    exponent: ac.u8 = value[10:18]
    fraction: ac.u10 = value[:10]
    fraction64: ac.u64 = fraction
    zeros: ac.u7 = ac.count_leading_zeros(fraction64)
    shift: ac.u6 = zeros[:6] - 53
    shift = 0 if (exponent != 0) | (fraction == 0) else shift
    normalized: ac.u64 = fraction64 << shift
    exponent13: ac.u13 = exponent
    shift13: ac.u13 = shift
    double_exponent: ac.u13 = exponent13 + 896 if exponent != 0 else 897 - shift13
    exponent64: ac.u64 = double_exponent
    sign64: ac.u64 = sign
    packed: ac.u64 = (sign64 << 63) | (exponent64 << 52) | ((normalized & 1023) << 42)
    return sign64 << 63 if (exponent == 0) & (fraction == 0) else packed


@ac.module
def FP19FiniteExpand(value: ac.u19) -> ac.u64:  # noqa: N802
    return expand_finite19(value)
