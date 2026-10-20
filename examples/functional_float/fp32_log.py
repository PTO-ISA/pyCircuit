"""Binary32 natural logarithm using a bounded Q80 atanh series.

This integer approximation is independent of host floating-point evaluation.
It is not an exhaustive correctly-rounded transcendental certification.
"""

import pycircuit as ac
from functional_float.float_formats import round_format, unpack_float
from functional_float.fp64_types import FloatWordResponse
from functional_float.fp32_log_step import LogState, log_step, multiply_shift80


def log_bits(value: ac.u32, request_valid: ac.u1) -> FloatWordResponse:
    word: ac.u64 = value
    parts = unpack_float(word, 0)
    zeros: ac.u7 = ac.count_leading_zeros(parts.significand)
    shift: ac.u6 = zeros[:6] - 40
    shift = 0 if parts.zero else shift
    significand: ac.u64 = parts.significand << shift
    m: ac.bits[128] = significand
    one128: ac.bits[128] = 8388608
    numerator: ac.bits[128] = (m - one128) << 80
    divisor: ac.bits[128] = m + one128
    z: ac.bits[128] = numerator // divisor
    z2: ac.bits[128] = multiply_shift80(z, z)
    state = LogState(term=z, total=z, denominator=3)
    for _iteration in range(24):
        state = log_step(state, z2)
    ln_m: ac.bits[128] = state.total << 1
    shift13: ac.u13 = shift
    exponent_bias: ac.u13 = parts.lsb_bias + 23 - shift13
    negative_exponent: ac.u1 = exponent_bias < 4096
    exponent: ac.u13 = (
        4096 - exponent_bias if negative_exponent else exponent_bias - 4096
    )
    exponent_wide: ac.bits[128] = exponent
    ln_two: ac.bits[128] = 837963523372001241319907
    exponent_term: ac.bits[128] = exponent_wide * ln_two
    negative: ac.u1 = negative_exponent & (exponent_term > ln_m)
    difference: ac.bits[128] = (
        exponent_term - ln_m if negative else ln_m - exponent_term
    )
    magnitude: ac.bits[128] = difference if negative_exponent else ln_m + exponent_term
    rounded = round_format(negative, magnitude, 4016, 0)
    invalid: ac.u1 = parts.snan | (parts.sign & ~parts.zero & ~parts.nan)
    any_nan: ac.u1 = parts.nan | invalid
    special: ac.u1 = any_nan | parts.inf | parts.zero
    result: ac.u64 = rounded.result
    result = 2139095040 if parts.inf else result
    result = 4286578688 if parts.zero else result
    result = 2143289344 if any_nan else result
    finite: ac.u1 = ~special
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=rounded.overflow & finite,
        underflow=rounded.underflow & finite,
        inexact=finite & (value != 1065353216),
        zero=finite & rounded.zero,
        divide_by_zero=parts.zero,
    )


@ac.module
def FP32Log(value: ac.u32, request_valid: ac.u1) -> FloatWordResponse:  # noqa: N802
    return log_bits(value, request_valid)
