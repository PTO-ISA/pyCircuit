"""Binary32/binary64 RNE square root through a restoring integer square root."""

import pycircuit as ac
from functional_float.fp32_product import round_pack
from functional_float.float_sqrt_step import SqrtState, digit_step
from functional_float.fp64_round import round_fp64
from functional_float.fp64_types import FloatWordResponse


def square_root(
    value: ac.u64, wide64: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:
    fraction_bits: ac.u6 = 52 if wide64 else 23
    exponent_mask: ac.u64 = 2047 if wide64 else 255
    exponent_raw: ac.u64 = (value >> fraction_bits) & exponent_mask
    exponent: ac.u13 = exponent_raw[:13]
    one: ac.u64 = 1
    fraction: ac.u64 = value & ((one << fraction_bits) - 1)
    mantissa: ac.u64 = fraction | (one << fraction_bits) if exponent != 0 else fraction
    sign_position: ac.u6 = 63 if wide64 else 31
    sign: ac.u1 = (value >> sign_position)[0:1]
    zero: ac.u1 = (exponent == 0) & (fraction == 0)
    special: ac.u1 = exponent_raw == exponent_mask
    nan: ac.u1 = special & (fraction != 0)
    quiet_position: ac.u6 = fraction_bits - 1
    quiet: ac.u1 = (fraction >> quiet_position)[0:1]
    invalid: ac.u1 = (nan & ~quiet) | (sign & ~zero & ~nan)
    zeros: ac.u7 = ac.count_leading_zeros(mantissa)
    excess: ac.u7 = 11 if wide64 else 40
    leading: ac.u7 = zeros - excess
    normalized: ac.bits[128] = mantissa
    normalized = normalized << leading
    effective_exponent: ac.u13 = exponent if exponent != 0 else 1
    exponent_offset: ac.u13 = 1025 if wide64 else 129
    leading13: ac.u13 = leading
    exponent_biased: ac.u13 = effective_exponent + exponent_offset - leading13
    odd: ac.u7 = exponent_biased[0:1]
    scale: ac.u7 = 74 if wide64 else 103
    radicand: ac.bits[128] = normalized << (scale + odd)
    state = SqrtState(root=0, remainder=0, radicand=radicand)
    for _step in range(55):
        state = digit_step(state)
    sticky: ac.u64 = 1 if (state.remainder != 0) | (state.radicand != 0) else 0
    root: ac.u64 = state.root | sticky
    magnitude: ac.bits[128] = root
    bias64: ac.u13 = 3018 + (exponent_biased >> 1)
    bias32_wide: ac.u13 = 330 + (exponent_biased >> 1)
    bias32: ac.u11 = bias32_wide[:11]
    rounded64 = round_fp64(0, magnitude, bias64)
    rounded32 = round_pack(0, root, bias32)
    narrow_result: ac.u64 = rounded32.result
    result: ac.u64 = rounded64.result if wide64 else narrow_result
    infinity: ac.u64 = 9218868437227405312 if wide64 else 2139095040
    qnan: ac.u64 = 9221120237041090560 if wide64 else 2143289344
    sign64: ac.u64 = sign
    result = infinity if special & ~nan else result
    result = sign64 << sign_position if zero else result
    result = qnan if nan | invalid else result
    inexact: ac.u1 = rounded64.inexact if wide64 else rounded32.inexact
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=0,
        underflow=0,
        inexact=inexact & ~special & ~invalid & ~zero,
        zero=zero,
        divide_by_zero=0,
    )


@ac.module
def FloatSqrt(  # noqa: N802
    value: ac.u64,
    wide64: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return square_root(value, wide64, request_valid)
