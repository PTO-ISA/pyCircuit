"""Exact raw FP8 E4M3FN/E5M2 and dynamic HiF8 expansion to binary32."""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse


def decode_lowp(value: ac.u8, kind: ac.u2, request_valid: ac.u1) -> FloatWordResponse:
    sign: ac.u1 = value[7:8]
    sign32: ac.u32 = sign
    e5: ac.u1 = kind == 1
    fraction_width: ac.u7 = 2 if e5 else 3
    exponent_mask: ac.u8 = 31 if e5 else 15
    exponent: ac.u8 = (value >> fraction_width) & exponent_mask
    one: ac.u64 = 1
    word: ac.u64 = value
    fraction: ac.u64 = word & ((one << fraction_width) - 1)
    significand: ac.u64 = (
        fraction | (one << fraction_width) if exponent != 0 else fraction
    )
    zeros: ac.u7 = ac.count_leading_zeros(significand)
    base: ac.u7 = 61 if e5 else 60
    shift: ac.u7 = zeros - base
    shift = 0 if significand == 0 else shift
    normalized: ac.u64 = significand << shift
    exponent13: ac.u13 = exponent
    shift13: ac.u13 = shift
    bias_delta: ac.u13 = 112 if e5 else 120
    normal_exp: ac.u13 = (
        exponent13 + bias_delta if exponent != 0 else bias_delta + 1 - shift13
    )
    exp32: ac.u32 = normal_exp
    one32: ac.u32 = 1
    frac32: ac.u32 = normalized[:32] & ((one32 << fraction_width) - 1)
    raw: ac.u32 = (sign32 << 31) | (exp32 << 23) | (frac32 << (23 - fraction_width))
    zero: ac.u1 = (exponent == 0) & (fraction == 0)
    raw = sign32 << 31 if zero else raw
    nan: ac.u1 = (exponent == exponent_mask) & (
        (fraction != 0) if e5 else (fraction == 7)
    )
    inf: ac.u1 = e5 & (exponent == 31) & (fraction == 0)
    snan: ac.u1 = e5 & nan & ~fraction[1:2]
    raw = (sign32 << 31) | 2139095040 if inf else raw
    raw = 2143289344 if nan else raw
    body: ac.u7 = value[:7]
    body8: ac.u8 = body
    bit3: ac.u8 = body[3:4]
    exp_h: ac.u8 = 127
    frac_h: ac.u8 = 0
    precision: ac.u7 = 3
    distance: ac.u8 = 0
    negative_exp: ac.u1 = 0
    if body < 8:
        exp_h = body8 + 104
        precision = 0
    elif body < 16:
        frac_h = body8 - 8
    elif body < 32:
        exp_h = 126 if body[3:4] else 128
        frac_h = body8 & 7
    else:
        negative_exp = body[4:5]
        if body < 64:
            distance = 2 + bit3
            frac_h = body8 & 7
        elif body < 96:
            distance = 4 + ((body8 >> 2) & 3)
            frac_h = body8 & 3
            precision = 2
        else:
            distance = 8 + ((body8 >> 1) & 7)
            frac_h = body8 & 1
            precision = 1
        exp_h = 127 - distance if negative_exp else 127 + distance
    exp_h32: ac.u32 = exp_h
    frac_h32: ac.u32 = frac_h
    hif: ac.u32 = (sign32 << 31) | (exp_h32 << 23) | (frac_h32 << (23 - precision))
    hif = 0 if body == 0 else hif
    hif = 2143289344 if value == 128 else hif
    hif = 2139095040 if value == 111 else hif
    hif = 4286578688 if value == 239 else hif
    raw = hif if kind == 2 else raw
    scale: ac.u32 = value
    scale = scale << 23
    scale = 4194304 if value == 0 else scale
    scale = 2143289344 if value == 255 else scale
    raw = scale if kind == 3 else raw
    invalid: ac.u1 = snan & (kind == 1)
    result: ac.u64 = raw
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=0,
        underflow=0,
        inexact=0,
        zero=raw[:31] == 0,
        divide_by_zero=0,
    )


@ac.module
def LowpDecode(value: ac.u8, kind: ac.u2, request_valid: ac.u1) -> FloatWordResponse:  # noqa: N802
    return decode_lowp(value, kind, request_valid)
