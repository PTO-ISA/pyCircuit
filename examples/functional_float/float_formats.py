"""Exact IEEE payload interpretation and common integer RNE packing.

Format selectors: binary32=0, binary64=1, binary16=2, bfloat16=3. The packer
rounds magnitude * 2**(lsb_bias-4096) once, with post-rounding tininess.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse


@ac.struct
class FloatParts:
    sign: ac.u1
    significand: ac.u64
    lsb_bias: ac.u13
    zero: ac.u1
    nan: ac.u1
    inf: ac.u1
    snan: ac.u1


def fraction_bits(format: ac.u2) -> ac.u7:  # noqa: A002
    value: ac.u7 = 52 if format == 1 else 23
    value = 10 if format == 2 else value
    return 7 if format == 3 else value


def exponent_bias(format: ac.u2) -> ac.u13:  # noqa: A002
    value: ac.u13 = 1023 if format == 1 else 127
    return 15 if format == 2 else value


def exponent_max(format: ac.u2) -> ac.u13:  # noqa: A002
    value: ac.u13 = 2047 if format == 1 else 255
    return 31 if format == 2 else value


def sign_position(format: ac.u2) -> ac.u6:  # noqa: A002
    value: ac.u6 = 63 if format == 1 else 31
    return 15 if (format == 2) | (format == 3) else value


def unpack_float(value: ac.u64, format: ac.u2) -> FloatParts:  # noqa: A002
    fraction_width: ac.u7 = fraction_bits(format)
    one: ac.u64 = 1
    fraction: ac.u64 = value & ((one << fraction_width) - 1)
    maximum: ac.u64 = exponent_max(format)
    exponent_raw: ac.u64 = (value >> fraction_width) & maximum
    exponent: ac.u13 = exponent_raw[:13]
    significand: ac.u64 = (
        fraction | (one << fraction_width) if exponent != 0 else fraction
    )
    effective: ac.u13 = exponent if exponent != 0 else 1
    width13: ac.u13 = fraction_width
    bias: ac.u13 = exponent_bias(format)
    lsb: ac.u13 = effective + 4096 - bias - width13
    position: ac.u6 = sign_position(format)
    sign: ac.u1 = (value >> position)[0:1]
    special: ac.u1 = exponent_raw == maximum
    nan: ac.u1 = special & (fraction != 0)
    quiet_position: ac.u7 = fraction_width - 1
    quiet: ac.u1 = (fraction >> quiet_position)[0:1]
    return FloatParts(
        sign=sign,
        significand=significand,
        lsb_bias=lsb,
        zero=(exponent == 0) & (fraction == 0),
        nan=nan,
        inf=special & (fraction == 0),
        snan=nan & ~quiet,
    )


def pack_float(
    sign: ac.u1,
    magnitude: ac.bits[128],
    lsb_bias: ac.u13,
    precision: ac.u7,
    bias: ac.u13,
    maximum: ac.u13,
    position: ac.u6,
) -> FloatWordResponse:
    precision14: ac.u14 = precision
    bias14: ac.u14 = bias
    maximum14: ac.u14 = maximum
    zeros: ac.u8 = ac.count_leading_zeros(magnitude)
    leading: ac.u8 = 127 - zeros
    leading = 0 if magnitude == 0 else leading
    leading14: ac.u14 = leading
    source_lsb: ac.u14 = lsb_bias
    exponent_plus: ac.u14 = source_lsb + leading14
    minimum: ac.u14 = 4097 - bias14
    normal: ac.u1 = exponent_plus >= minimum
    normal_shift: ac.u14 = leading14 + 4096 - precision14
    subnormal_shift: ac.u14 = 8193 - bias14 - precision14 - source_lsb
    shift_bias: ac.u14 = normal_shift if normal else subnormal_shift
    right: ac.u1 = shift_bias >= 4096
    right14: ac.u14 = shift_bias - 4096 if right else 0
    left14: ac.u14 = 4096 - shift_bias if ~right else 0
    right_amount: ac.u8 = 255 if right14 > 255 else right14[:8]
    left_amount: ac.u7 = 127 if left14 > 127 else left14[:7]
    truncated: ac.bits[128] = (
        magnitude >> right_amount if right else magnitude << left_amount
    )
    guard14: ac.u14 = right14 - 1
    guard_amount: ac.u8 = 255 if guard14 > 255 else guard14[:8]
    guard: ac.u1 = (magnitude >> guard_amount)[0:1]
    guard = guard if right & (right14 >= 1) else 0
    one: ac.bits[128] = 1
    lower_mask: ac.bits[128] = (one << guard_amount) - 1
    sticky: ac.u1 = (magnitude & lower_mask) != 0
    discarded: ac.u1 = (guard | sticky) if right & (right14 >= 1) else 0
    increment: ac.bits[128] = 1 if guard & (sticky | truncated[0:1]) else 0
    rounded: ac.bits[128] = truncated + increment
    carry_shift: ac.u7 = precision + 1
    normal_carry: ac.u1 = (rounded >> carry_shift)[0:1]
    subnormal_carry: ac.u1 = (rounded >> precision)[0:1]
    normal_exponent: ac.u14 = exponent_plus + bias14 - 4096
    normal_exponent = normal_exponent + 1 if normal_carry else normal_exponent
    subnormal_exponent: ac.u14 = 1 if subnormal_carry else 0
    exponent: ac.u14 = normal_exponent if normal else subnormal_exponent
    overflow: ac.u1 = (exponent >= maximum14) & (magnitude != 0)
    exponent = maximum14 if overflow else exponent
    one64: ac.u64 = 1
    fraction_mask: ac.u64 = (one64 << precision) - 1
    rounded64: ac.u64 = rounded[:64]
    fraction: ac.u64 = rounded64 & fraction_mask
    carry: ac.u1 = normal_carry if normal else subnormal_carry
    fraction = 0 if carry | overflow else fraction
    exponent64: ac.u64 = exponent
    sign64: ac.u64 = sign
    packed: ac.u64 = (sign64 << position) | (exponent64 << precision) | fraction
    zero: ac.u1 = (magnitude == 0) | (rounded == 0)
    result: ac.u64 = sign64 << position if zero else packed
    return FloatWordResponse(
        result=result,
        valid=1,
        invalid=0,
        overflow=overflow,
        underflow=(exponent == 0) & discarded,
        inexact=discarded | overflow,
        zero=zero,
        divide_by_zero=0,
    )


def round_format(
    sign: ac.u1, magnitude: ac.bits[128], lsb_bias: ac.u13, format: ac.u2  # noqa: A002
) -> FloatWordResponse:  # noqa: A002
    return pack_float(
        sign,
        magnitude,
        lsb_bias,
        fraction_bits(format),
        exponent_bias(format),
        exponent_max(format),
        sign_position(format),
    )
