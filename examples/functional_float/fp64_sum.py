"""Binary64 RNE sum with integer alignment and sticky discarded bits."""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.fp64_round import unpack_fp64, round_fp64


def shift_jam(value: ac.bits[128], amount: ac.u12) -> ac.bits[128]:
    shift: ac.u8 = 255 if amount > 255 else amount[:8]
    one: ac.bits[128] = 1
    mask: ac.bits[128] = (one << shift) - 1
    sticky: ac.bits[128] = 1 if (value & mask) != 0 else 0
    return (value >> shift) | sticky


def sum_fp64(lhs: ac.u64, rhs: ac.u64, request_valid: ac.u1) -> FloatWordResponse:
    a = unpack_fp64(lhs)
    b = unpack_fp64(rhs)
    ae: ac.u12 = a.exponent
    be: ac.u12 = b.exponent
    high: ac.u12 = ae if ae >= be else be
    am: ac.bits[128] = a.mantissa
    bm: ac.bits[128] = b.mantissa
    aligned_a = shift_jam(am << 74, high - ae)
    aligned_b = shift_jam(bm << 74, high - be)
    same_sign: ac.u1 = a.sign == b.sign
    a_larger: ac.u1 = aligned_a >= aligned_b
    difference: ac.bits[128] = (
        aligned_a - aligned_b if a_larger else aligned_b - aligned_a
    )
    magnitude: ac.bits[128] = aligned_a + aligned_b if same_sign else difference
    difference_sign: ac.u1 = a.sign if a_larger else b.sign
    sign: ac.u1 = a.sign if same_sign else difference_sign
    sign = 0 if (magnitude == 0) & ~same_sign else sign
    high13: ac.u13 = high
    rounded = round_fp64(sign, magnitude, high13 + 2947)
    invalid: ac.u1 = a.snan | b.snan | (a.inf & b.inf & (a.sign != b.sign))
    any_nan: ac.u1 = a.nan | b.nan | invalid
    any_inf: ac.u1 = a.inf | b.inf
    infinity_sign: ac.u64 = a.sign if a.inf else b.sign
    infinity: ac.u64 = (infinity_sign << 63) | 9218868437227405312
    result: ac.u64 = infinity if any_inf else rounded.result
    result = 9221120237041090560 if any_nan else result
    finite: ac.u1 = ~(any_nan | any_inf)
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=rounded.overflow & finite,
        underflow=rounded.underflow & finite,
        inexact=rounded.inexact & finite,
        zero=result[:63] == 0,
        divide_by_zero=0,
    )


@ac.module
def FP64Sum(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, request_valid: ac.u1
) -> FloatWordResponse:
    return sum_fp64(lhs, rhs, request_valid)
