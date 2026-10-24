"""Bit-level binary32 -> bfloat16 RNE conversion source unit.

BF16 keeps the binary32 sign and exponent and rounds the 23-bit fraction to
seven bits with round-to-nearest, ties-to-even.  The implementation uses the
standard integer bias form and only fixed-width bits; no host floating point
or legacy arithmetic is used.
"""

import pycircuit as ac

from functional_float.bf16_types import BF16Response


def convert(value: ac.u32, request_valid: ac.u1) -> BF16Response:
    sign: ac.u1 = value[31:32]
    exponent: ac.u8 = value[23:31]
    fraction: ac.u23 = value[:23]
    frac_zero: ac.u1 = fraction == 0
    is_zero: ac.u1 = (exponent == 0) & frac_zero
    is_inf: ac.u1 = (exponent == 255) & frac_zero
    is_nan: ac.u1 = (exponent == 255) & (~frac_zero)
    is_snan: ac.u1 = is_nan & (~fraction[22:23])

    high_lsb: ac.u1 = value[16:17]
    high_lsb64: ac.u64 = high_lsb
    bias: ac.u64 = 0x7FFF + high_lsb64
    extended: ac.u64 = value
    rounded: ac.u64 = extended + bias
    rounded16: ac.u16 = rounded[16:32]

    sign16: ac.u16 = sign
    sign_field: ac.u16 = sign16 << 15
    qnan: ac.u16 = sign_field | 0x7FC0
    inf: ac.u16 = sign_field | 0x7F80
    magnitude: ac.u15 = rounded16[0:15]
    overflow_raw: ac.u1 = magnitude == 0x7F80
    overflow: ac.u1 = overflow_raw & (~(is_inf | is_nan))
    inexact_raw: ac.u1 = value[:16] != 0
    inexact: ac.u1 = (inexact_raw | overflow) & (~(is_zero | is_inf | is_nan))
    underflow: ac.u1 = (rounded16[7:15] == 0) & inexact & (~(is_zero | is_inf | is_nan))
    zero: ac.u1 = (magnitude == 0) & (~(is_nan | is_inf))
    result: ac.u16 = qnan if is_nan else (inf if is_inf else rounded16)
    invalid: ac.u1 = request_valid & is_snan
    valid: ac.u1 = request_valid & (~invalid)
    return BF16Response(
        result=result,
        valid=valid,
        invalid=invalid,
        overflow=overflow,
        underflow=underflow,
        inexact=inexact,
        zero=zero,
    )


@ac.module
def FP32ToBF16(value: ac.u32, request_valid: ac.u1) -> BF16Response:  # noqa: N802
    return convert(value, request_valid)
