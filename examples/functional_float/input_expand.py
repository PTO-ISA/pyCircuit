"""Exact binary16/bfloat16 raw expansion, without host floating point."""

import pycircuit as ac


def expand(value: ac.u16, bfloat: ac.u1) -> ac.u32:
    sign: ac.u32 = value[15:16]
    sign = sign << 31
    fraction: ac.u32 = value[:10]
    exponent: ac.u8 = value[10:15]
    normal_exp: ac.u32 = exponent + 112
    result: ac.u32 = sign | (normal_exp << 23) | (fraction << 13)
    small_fraction: ac.u10 = value[:10]
    leading: ac.u4 = ac.count_leading_zeros(small_fraction)
    shift: ac.u5 = leading
    shift = shift + 1
    sub_exp: ac.u32 = 112
    leading_wide: ac.u32 = leading
    sub_exp = sub_exp - leading_wide
    sub_mantissa: ac.u32 = (fraction << shift) & 1023
    subnormal: ac.u32 = sign | (sub_exp << 23) | (sub_mantissa << 13)
    subnormal = sign if fraction == 0 else subnormal
    result = subnormal if exponent == 0 else result
    special: ac.u32 = sign | 2139095040 | (fraction << 13)
    result = special if exponent == 31 else result
    bf: ac.u32 = value
    bf = bf << 16
    return bf if bfloat else result


@ac.module
def InputExpand(value: ac.u16, bfloat: ac.u1) -> ac.u32:  # noqa: N802
    return expand(value, bfloat)
