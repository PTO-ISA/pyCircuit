"""Positive exponent-only E8M0 encoding with explicit exponent rounding.

Modes0..6: nearest-even, floor, ceiling, toward-zero, nearest-away, odd,
hybrid ties-to-positive-infinity. Zero/negative/NaN inputs return invalid FF.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.float_formats import unpack_float


def encode_scale(
    value: ac.u32, rounding: ac.u3, saturating: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:
    word: ac.u64 = value
    source = unpack_float(word, 0)
    zeroes: ac.u7 = ac.count_leading_zeros(source.significand)
    highest: ac.u7 = 63 - zeroes
    highest = 0 if source.zero else highest
    high13: ac.u13 = highest
    exponent: ac.u13 = source.lsb_bias + high13
    one: ac.u64 = 1
    exact: ac.u1 = source.significand == (one << highest)
    sig128: ac.bits[128] = source.significand
    square: ac.bits[128] = sig128 * sig128
    one128: ac.bits[128] = 1
    h8: ac.u8 = highest
    boundary: ac.bits[128] = one128 << ((h8 << 1) + 1)
    above: ac.u1 = square > boundary
    tie: ac.u1 = square == boundary
    negative: ac.u1 = exponent < 4096
    increment: ac.u1 = above | (tie & exponent[0:1])
    increment = 0 if rounding == 1 else increment
    increment = ~exact if rounding == 2 else increment
    increment = negative & ~exact if rounding == 3 else increment
    increment = above | (tie & ~negative) if rounding == 4 else increment
    increment = ~exponent[0:1] & ~exact if rounding == 5 else increment
    increment = above | tie if rounding == 6 else increment
    increment13: ac.u13 = increment
    encoded: ac.u13 = exponent - 3969 + increment13
    invalid: ac.u1 = source.nan | source.sign | source.zero
    underflow: ac.u1 = (exponent < 3969) & ~invalid
    overflow: ac.u1 = ((exponent == 4223) & ~exact) | source.inf
    overflow = overflow & ~invalid
    result: ac.u64 = encoded[:8]
    result = 0 if underflow & saturating else result
    result = 254 if overflow & saturating else result
    result = 255 if invalid | ((underflow | overflow) & ~saturating) else result
    supported: ac.u1 = rounding != 7
    return FloatWordResponse(
        result=result if supported else 0,
        valid=request_valid & supported & ~invalid,
        invalid=invalid & supported,
        overflow=overflow & supported,
        underflow=underflow & supported,
        inexact=(~exact | overflow | underflow) & ~invalid & supported,
        zero=0,
        divide_by_zero=0,
    )


@ac.module
def E8M0Encode(  # noqa: N802
    value: ac.u32, rounding: ac.u3, saturating: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:  # noqa: N802
    return encode_scale(value, rounding, saturating, request_valid)
