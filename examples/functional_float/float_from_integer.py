# ruff: noqa: A002
"""RNE integer conversion directly to binary32/64/16/bfloat16 bit payloads.

All formats round once from the original integer magnitude; no host float or
intermediate binary32 conversion supplies the answer.
"""
import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse


def from_integer(
    value: ac.u64, unsigned_input: ac.u1, format: ac.u2, request_valid: ac.u1
) -> FloatWordResponse:
    sign: ac.u1 = value[63:64] & ~unsigned_input
    magnitude: ac.u64 = 0 - value if sign else value
    zeros: ac.u7 = ac.count_leading_zeros(magnitude)
    leading: ac.u7 = 63 - zeros
    leading = 0 if magnitude == 0 else leading
    precision: ac.u7 = 23
    precision = 52 if format == 1 else precision
    precision = 10 if format == 2 else precision
    precision = 7 if format == 3 else precision
    bias: ac.u12 = 1023 if format == 1 else 127
    bias = 15 if format == 2 else bias
    maximum: ac.u12 = 2047 if format == 1 else 255
    maximum = 31 if format == 2 else maximum
    sign_position: ac.u6 = 63 if format == 1 else 31
    sign_position = 15 if (format == 2) | (format == 3) else sign_position
    right: ac.u1 = leading >= precision
    right_amount: ac.u7 = leading - precision if right else 0
    left_amount: ac.u7 = precision - leading if ~right else 0
    truncated: ac.u64 = magnitude >> right_amount if right else magnitude << left_amount
    guard_amount: ac.u7 = right_amount - 1
    guard: ac.u1 = (magnitude >> guard_amount)[0:1]
    guard = guard if right & (right_amount != 0) else 0
    one: ac.u64 = 1
    mask: ac.u64 = (one << guard_amount) - 1
    sticky: ac.u1 = (magnitude & mask) != 0
    discarded: ac.u1 = guard | sticky if right & (right_amount != 0) else 0
    increment: ac.u64 = 1 if guard & (sticky | truncated[0:1]) else 0
    rounded: ac.u64 = truncated + increment
    carry_shift: ac.u7 = precision + 1
    carry: ac.u1 = (rounded >> carry_shift)[0:1]
    normalized: ac.u64 = rounded >> 1 if carry else rounded
    leading12: ac.u12 = leading
    exponent: ac.u12 = leading12 + bias
    exponent = exponent + 1 if carry else exponent
    overflow: ac.u1 = exponent >= maximum
    exponent = maximum if overflow else exponent
    fraction_mask: ac.u64 = (one << precision) - 1
    fraction: ac.u64 = normalized & fraction_mask
    fraction = 0 if overflow else fraction
    exponent64: ac.u64 = exponent
    sign64: ac.u64 = sign
    result: ac.u64 = (sign64 << sign_position) | (exponent64 << precision) | fraction
    result = 0 if magnitude == 0 else result
    return FloatWordResponse(
        result=result,
        valid=request_valid,
        invalid=0,
        overflow=overflow,
        underflow=0,
        inexact=discarded | overflow,
        zero=magnitude == 0,
        divide_by_zero=0,
    )


@ac.module
def FloatFromInteger(  # noqa: N802
    value: ac.u64, unsigned_input: ac.u1, format: ac.u2, request_valid: ac.u1
) -> FloatWordResponse:  # noqa: N802
    return from_integer(value, unsigned_input, format, request_valid)
