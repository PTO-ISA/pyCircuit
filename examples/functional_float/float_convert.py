"""One-round raw IEEE conversion among binary32/64/16 and bfloat16."""

import pycircuit as ac
from functional_float.float_formats import (
    exponent_max,
    fraction_bits,
    sign_position,
    unpack_float,
    round_format,
)
from functional_float.fp64_types import FloatWordResponse


def convert_bits(
    value: ac.u64, source_format: ac.u2, destination_format: ac.u2, request_valid: ac.u1
) -> FloatWordResponse:
    parts = unpack_float(value, source_format)
    magnitude: ac.bits[128] = parts.significand
    rounded = round_format(parts.sign, magnitude, parts.lsb_bias, destination_format)
    precision: ac.u7 = fraction_bits(destination_format)
    maximum: ac.u64 = exponent_max(destination_format)
    sign: ac.u64 = parts.sign
    position: ac.u6 = sign_position(destination_format)
    infinity: ac.u64 = (sign << position) | (maximum << precision)
    one: ac.u64 = 1
    quiet_position: ac.u7 = precision - 1
    qnan: ac.u64 = (maximum << precision) | (one << quiet_position)
    result: ac.u64 = infinity if parts.inf else rounded.result
    result = qnan if parts.nan else result
    finite: ac.u1 = ~(parts.nan | parts.inf)
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~parts.snan,
        invalid=parts.snan,
        overflow=rounded.overflow & finite,
        underflow=rounded.underflow & finite,
        inexact=rounded.inexact & finite,
        zero=parts.zero | (rounded.zero & finite),
        divide_by_zero=0,
    )


@ac.module
def FloatConvert(  # noqa: N802
    value: ac.u64,
    source_format: ac.u2,
    destination_format: ac.u2,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return convert_bits(value, source_format, destination_format, request_valid)
