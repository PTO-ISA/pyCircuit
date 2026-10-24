"""Binary64 separately rounded product; all arithmetic uses captured bits."""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.fp64_round import unpack_fp64, round_fp64


def product_fp64(lhs: ac.u64, rhs: ac.u64, request_valid: ac.u1) -> FloatWordResponse:
    a = unpack_fp64(lhs)
    b = unpack_fp64(rhs)
    am: ac.bits[128] = a.mantissa
    bm: ac.bits[128] = b.mantissa
    ae: ac.u13 = a.exponent
    be: ac.u13 = b.exponent
    sign: ac.u1 = a.sign ^ b.sign
    rounded = round_fp64(sign, am * bm, ae + be + 1946)
    invalid: ac.u1 = a.snan | b.snan | (a.inf & b.zero) | (a.zero & b.inf)
    any_nan: ac.u1 = a.nan | b.nan | invalid
    any_inf: ac.u1 = a.inf | b.inf
    sign64: ac.u64 = sign
    infinity: ac.u64 = (sign64 << 63) | 9218868437227405312
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
def FP64Product(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, request_valid: ac.u1
) -> FloatWordResponse:
    return product_fp64(lhs, rhs, request_valid)
