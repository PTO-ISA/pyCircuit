"""Rounded floating to configurable integer bits, with modular/saturating output.

Finite out-of-range values set OF|NX and wrap or saturate as selected. NaN
returns zero with NV; infinity returns the signed endpoint with NV. This is
separate from FloatToInteger's endpoint/NV policy. Widths 1..64 and rounding
RNE/RTM/RTP/RTZ/RNA/ROD (0..5) are supported over binary32/binary64 raw input.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse


def integer_bits(
    value: ac.u64,
    wide64: ac.u1,
    unsigned_output: ac.u1,
    width: ac.u7,
    rounding: ac.u3,
    saturating: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    fraction_bits: ac.u6 = 52 if wide64 else 23
    exponent_mask: ac.u64 = 2047 if wide64 else 255
    exponent_raw: ac.u64 = (value >> fraction_bits) & exponent_mask
    exponent: ac.u13 = exponent_raw[:13]
    one: ac.u64 = 1
    fraction_mask: ac.u64 = (one << fraction_bits) - 1
    fraction: ac.u64 = value & fraction_mask
    mantissa: ac.u64 = fraction | (one << fraction_bits) if exponent != 0 else fraction
    sign_position: ac.u6 = 63 if wide64 else 31
    sign: ac.u1 = (value >> sign_position)[0:1]
    special: ac.u1 = exponent_raw == exponent_mask
    nan: ac.u1 = special & (fraction != 0)
    bias: ac.u13 = 1023 if wide64 else 127
    effective_exponent: ac.u13 = exponent if exponent != 0 else 1
    fraction_bits13: ac.u13 = fraction_bits
    lsb_bias: ac.u13 = effective_exponent + 4096 - bias - fraction_bits13
    left: ac.u1 = lsb_bias >= 4096
    left_amount: ac.u13 = lsb_bias - 4096 if left else 0
    right_amount: ac.u13 = 4096 - lsb_bias if ~left else 0
    right_amount = 127 if right_amount > 127 else right_amount
    left_amount_safe: ac.u13 = 127 if left_amount > 127 else left_amount
    magnitude: ac.bits[128] = mantissa
    truncated: ac.bits[128] = magnitude >> right_amount
    one_wide: ac.bits[128] = 1
    lower_mask: ac.bits[128] = (one_wide << right_amount) - 1
    discarded: ac.u1 = (magnitude & lower_mask) != 0
    guard_amount: ac.u13 = right_amount - 1
    guard: ac.u1 = (magnitude >> guard_amount)[0:1]
    guard = guard if right_amount != 0 else 0
    sticky_mask: ac.bits[128] = (one_wide << guard_amount) - 1
    sticky: ac.u1 = (magnitude & sticky_mask) != 0
    increment: ac.u1 = guard & (sticky | truncated[0:1])
    increment = sign & discarded if rounding == 1 else increment
    increment = ~sign & discarded if rounding == 2 else increment
    increment = 0 if rounding == 3 else increment
    increment = guard if rounding == 4 else increment
    increment_wide: ac.bits[128] = increment
    integral: ac.bits[128] = truncated | 1 if discarded else truncated
    rounded: ac.bits[128] = (
        magnitude << left_amount_safe
        if left
        else (integral if rounding == 5 else truncated + increment_wide)
    )
    maximum_bit: ac.u7 = width if unsigned_output else width - 1
    limit: ac.bits[128] = one_wide << maximum_bit
    overflow: ac.u1 = rounded >= limit
    negative_overflow: ac.u1 = rounded > limit
    signed_negative: ac.u1 = sign & ~unsigned_output
    overflow = negative_overflow if signed_negative else overflow
    maximum_bit13: ac.u13 = maximum_bit
    overflow = overflow | (
        (effective_exponent > bias + maximum_bit13) & (mantissa != 0)
    )
    overflow = overflow | (unsigned_output & sign & (rounded != 0))
    magnitude64: ac.u64 = rounded[:64]
    normal: ac.u64 = 0 - magnitude64 if sign else magnitude64
    maximum: ac.u64 = (one << maximum_bit) - 1
    minimum: ac.u64 = 0 if unsigned_output else 0 - (one << maximum_bit)
    endpoint: ac.u64 = minimum if sign else maximum
    selected: ac.u64 = endpoint if saturating & overflow else normal
    selected = endpoint if special else selected
    selected = 0 if nan else selected
    mask: ac.u64 = (one << width) - 1
    selected = selected & mask
    top: ac.u7 = width - 1
    negative: ac.u1 = (selected >> top)[0:1] & ~unsigned_output
    selected = selected | ~mask if negative else selected
    inexact: ac.u1 = (discarded & ~left) | overflow
    return FloatWordResponse(
        result=selected,
        valid=request_valid & (width >= 1) & (width <= 64) & (rounding <= 5),
        invalid=special,
        overflow=overflow & ~special,
        underflow=0,
        inexact=inexact & ~special,
        zero=selected == 0,
        divide_by_zero=0,
    )


@ac.module
def FloatToIntegerBits(  # noqa: N802
    value: ac.u64,
    wide64: ac.u1,
    unsigned_output: ac.u1,
    width: ac.u7,
    rounding: ac.u3,
    saturating: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return integer_bits(
        value, wide64, unsigned_output, width, rounding, saturating, request_valid
    )
