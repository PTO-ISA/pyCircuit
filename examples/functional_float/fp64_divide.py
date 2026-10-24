"""Binary64 quotient from normalized 128-bit integer division and remainder."""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.fp64_round import unpack_fp64, round_fp64


def divide_fp64(lhs: ac.u64, rhs: ac.u64, request_valid: ac.u1) -> FloatWordResponse:
    a = unpack_fp64(lhs)
    b = unpack_fp64(rhs)
    az: ac.u6 = ac.count_leading_zeros(a.mantissa)
    bz: ac.u6 = ac.count_leading_zeros(b.mantissa)
    am: ac.bits[128] = a.mantissa
    bm: ac.bits[128] = b.mantissa
    normalized_a: ac.bits[128] = am << az
    normalized_b: ac.bits[128] = bm << bz
    denominator: ac.bits[128] = 1 if b.zero else normalized_b
    numerator: ac.bits[128] = normalized_a << 64
    quotient: ac.bits[128] = numerator // denominator
    remainder: ac.bits[128] = numerator % denominator
    sticky: ac.bits[128] = 1 if remainder != 0 else 0
    ae: ac.u13 = a.exponent
    be: ac.u13 = b.exponent
    ashift: ac.u13 = az
    bshift: ac.u13 = bz
    sign: ac.u1 = a.sign ^ b.sign
    rounded = round_fp64(sign, quotient | sticky, ae - be + bshift - ashift + 4032)
    invalid: ac.u1 = a.snan | b.snan | (a.inf & b.inf) | (a.zero & b.zero)
    any_nan: ac.u1 = a.nan | b.nan | invalid
    special: ac.u1 = any_nan | a.inf | b.inf | a.zero | b.zero
    sign64: ac.u64 = sign
    result: ac.u64 = rounded.result
    result = sign64 << 63 if a.zero | b.inf else result
    result = (sign64 << 63) | 9218868437227405312 if a.inf | b.zero else result
    result = 9221120237041090560 if any_nan else result
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=rounded.overflow & ~special,
        underflow=rounded.underflow & ~special,
        inexact=rounded.inexact & ~special,
        zero=result[:63] == 0,
        divide_by_zero=(~a.nan) & (~b.nan) & (~a.inf) & (~a.zero) & b.zero,
    )


@ac.module
def FP64Divide(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, request_valid: ac.u1
) -> FloatWordResponse:
    return divide_fp64(lhs, rhs, request_valid)
