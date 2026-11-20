"""Finite binary32 arithmetic for the bounded FA2 profile.

Normal inputs and signed zero; subnormals flush to signed zero, overflow
saturates finite. Results truncate toward zero. NaNs/infinities are outside
this profile. exp accepts nonpositive arguments, clamps below -16, and uses
range reduction, a fourth-degree polynomial and eight squarings.
"""

import pycircuit as ac


def wide_value(x: ac.u32) -> ac.u64:
    return x


def shift32_value(x: ac.u7) -> ac.u32:
    return x


def fp_add_value(a: ac.u32, b: ac.u32) -> ac.u32:
    aa = a & 2147483647
    bb = b & 2147483647
    large = a
    small = b
    if aa < bb:
        large = b
        small = a
    ex = (large >> 23) & 255
    ey = (small >> 23) & 255
    mx = wide_value((large & 8388607) | 8388608) << 24
    my = wide_value((small & 8388607) | 8388608) << 24
    if ex == 0:
        mx = 0
    if ey == 0:
        my = 0
    delta = ex - ey
    if delta > 63:
        delta = 63
    unaligned = my | 0
    my = my >> delta
    if (my << delta) != unaligned:
        my = my | 1
    total = mx + my
    if ((large ^ small) & 2147483648) != 0:
        total = mx - my
    exponent = ex
    if total >= 281474976710656:
        total = total >> 1
        exponent = exponent + 1
    shift = shift32_value(ac.count_leading_zeros(total)) - 16
    if total != 0:
        total = total << shift
    result = large & 2147483648
    if total != 0:
        if exponent > shift:
            exponent = exponent - shift
            mantissa = (total >> 24) & 8388607
            result = result | (exponent << 23) | mantissa[:32]
            if exponent >= 255:
                result = (large & 2147483648) | 2139095039
    return result


def fp_mul_value(a: ac.u32, b: ac.u32) -> ac.u32:
    ea = (a >> 23) & 255
    eb = (b >> 23) & 255
    ma = wide_value((a & 8388607) | 8388608)
    mb = wide_value((b & 8388607) | 8388608)
    product = ma * mb
    exponent = ea + eb
    fraction = (product >> 23) & 8388607
    if product >= 140737488355328:
        fraction = (product >> 24) & 8388607
        exponent = exponent + 1
    result = (a ^ b) & 2147483648
    if ea != 0 and eb != 0 and exponent > 127:
        exponent = exponent - 127
        result = result | (exponent << 23) | fraction[:32]
        if exponent >= 255:
            result = ((a ^ b) & 2147483648) | 2139095039
    return result


@ac.module
def wide(x: ac.u32) -> ac.u64:
    return wide_value(x)


@ac.module
def shift32(x: ac.u7) -> ac.u32:
    return shift32_value(x)


@ac.module
def fp_add(a: ac.u32, b: ac.u32) -> ac.u32:
    return fp_add_value(a, b)


@ac.module
def fp_mul(a: ac.u32, b: ac.u32) -> ac.u32:
    return fp_mul_value(a, b)


@ac.module
def fp_div(a: ac.u32, b: ac.u32) -> ac.u32:
    ea = (a >> 23) & 255
    eb = (b >> 23) & 255
    ma = wide((a & 8388607) | 8388608)
    mb = wide((b & 8388607) | 8388608)
    exponent = ea + 127
    numerator = ma | 0
    if ma < mb:
        numerator = ma << 1
        exponent = exponent - 1
    quotient = (numerator << 23) // mb
    result = (a ^ b) & 2147483648
    if ea != 0 and eb != 0 and exponent > eb:
        exponent = exponent - eb
        fraction = quotient & 8388607
        result = result | (exponent << 23) | fraction[:32]
        if exponent >= 255:
            result = ((a ^ b) & 2147483648) | 2139095039
    return result


@ac.module
def decode16(x: ac.u16, bf16: ac.u1) -> ac.u32:
    raw = wide(x)
    sign = (raw & 32768) << 16
    exponent = (raw >> 10) & 31
    result = sign
    if exponent != 0:
        result = sign | ((exponent + 112) << 23) | ((raw & 1023) << 13)
    if bf16:
        result = raw << 16
        if (raw & 32640) == 0:
            result = sign
    return result[:32]


@ac.module
def encode16(x: ac.u32, bf16: ac.u1) -> ac.u16:
    exponent = (x >> 23) & 255
    result = (x >> 16) & 32768
    if exponent > 112:
        result = result | ((exponent - 112) << 10) | ((x >> 13) & 1023)
        if exponent >= 143:
            result = ((x >> 16) & 32768) | 31743
    if bf16:
        result = x >> 16
        if exponent == 0:
            result = result & 32768
    return result[:16]


@ac.module
def fp_exp_neg(x: ac.u32) -> ac.u32:
    y = fp_mul(x, 998244352)  # 1/256
    p0 = fp_add(1042983595, fp_mul(y, 1026206379))  # 1/6 + y/24
    p1 = fp_add(1056964608, fp_mul(y, p0))
    p2 = fp_add(1065353216, fp_mul(y, p1))
    p3 = fp_add(1065353216, fp_mul(y, p2))
    p4 = fp_mul(p3, p3)
    p5 = fp_mul(p4, p4)
    p6 = fp_mul(p5, p5)
    p7 = fp_mul(p6, p6)
    p8 = fp_mul(p7, p7)
    p9 = fp_mul(p8, p8)
    p10 = fp_mul(p9, p9)
    p11 = fp_mul(p10, p10)
    p = p11 | 0
    if (x & 2147483647) >= 1098907648:
        p = 0
    return p
