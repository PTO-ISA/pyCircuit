"""Direct binary32/binary64 to integer conversion with explicit rounding.

No host arithmetic supplies the result. Rounding selectors are RNE, RTM,
RTP, RTZ and nearest-away (0..4). Invalid conversion saturates to the
corresponding integer endpoint, with NV and no inexact flag.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse


def to_integer(
    value: ac.u64,
    wide64: ac.u1,
    unsigned_output: ac.u1,
    width_code: ac.u2,
    rounding: ac.u3,
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
    rounded: ac.bits[128] = (
        magnitude << left_amount_safe if left else truncated + increment_wide
    )
    width: ac.u7 = 64
    width = 32 if width_code == 1 else width
    width = 16 if width_code == 2 else width
    width = 8 if width_code == 3 else width
    maximum_bit: ac.u7 = width if unsigned_output else width - 1
    limit: ac.bits[128] = one_wide << maximum_bit
    signed_negative: ac.u1 = sign & ~unsigned_output
    overflow: ac.u1 = rounded >= limit
    negative_overflow: ac.u1 = rounded > limit
    overflow = negative_overflow if signed_negative else overflow
    maximum_bit13: ac.u13 = maximum_bit
    exponent_overflow: ac.u1 = effective_exponent > bias + maximum_bit13
    overflow = overflow | (exponent_overflow & (mantissa != 0))
    negative_unsigned: ac.u1 = unsigned_output & sign & (rounded != 0)
    invalid: ac.u1 = special | overflow | negative_unsigned
    magnitude64: ac.u64 = rounded[:64]
    normal_result: ac.u64 = 0 - magnitude64 if sign else magnitude64
    maximum: ac.u64 = (one << maximum_bit) - 1
    minimum: ac.u64 = 0 if unsigned_output else 0 - (one << maximum_bit)
    invalid_result: ac.u64 = minimum if sign & ~nan else maximum
    result: ac.u64 = invalid_result if invalid else normal_result
    inexact: ac.u1 = discarded & ~left & ~invalid
    return FloatWordResponse(
        result=result,
        valid=request_valid & (rounding <= 4),
        invalid=invalid,
        overflow=0,
        underflow=0,
        inexact=inexact,
        zero=result == 0,
        divide_by_zero=0,
    )


@ac.module
def FloatToInteger(  # noqa: N802
    value: ac.u64,
    wide64: ac.u1,
    unsigned_output: ac.u1,
    width_code: ac.u2,
    rounding: ac.u3,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return to_integer(
        value, wide64, unsigned_output, width_code, rounding, request_valid
    )
