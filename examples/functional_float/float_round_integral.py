"""Round binary32/64 to integer-valued results in the same floating format.

RNE, floor, ceiling, RTZ, RNA, odd and RHB are numeric policies0..6. Large
already-integral values bypass integer conversion. No host float is used.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.float_to_integer_bits import integer_bits
from functional_float.float_from_integer import from_integer


def round_integral(
    value: ac.u64,
    wide64: ac.u1,
    rounding: ac.u3,
    request_valid: ac.u1,
) -> FloatWordResponse:
    narrow_exponent: ac.u11 = value[23:31]
    exponent: ac.u11 = value[52:63] if wide64 else narrow_exponent
    maximum: ac.u11 = 2047 if wide64 else 255
    fraction_mask: ac.u64 = 4503599627370495 if wide64 else 8388607
    fraction: ac.u64 = value & fraction_mask
    sign: ac.u1 = value[63:64] if wide64 else value[31:32]
    quiet: ac.u1 = value[51:52] if wide64 else value[22:23]
    special: ac.u1 = exponent == maximum
    nan: ac.u1 = special & (fraction != 0)
    invalid: ac.u1 = nan & ~quiet
    sign_mask: ac.u64 = 9223372036854775808 if wide64 else 2147483648
    word_mask: ac.u64 = 18446744073709551615 if wide64 else 4294967295
    original: ac.u64 = value & word_mask
    absolute: ac.u64 = original & ~sign_mask
    boundary: ac.u64 = 4841369599423283200 if wide64 else 1258291200
    small: ac.u1 = ~special & (absolute < boundary)
    integral = integer_bits(value, wide64, 0, 64, rounding, 0, request_valid)
    fmt: ac.u2 = 1 if wide64 else 0
    encoded = from_integer(integral.result, 0, fmt, request_valid)
    rounded: ac.u64 = encoded.result
    rounded = sign_mask if sign & (integral.result == 0) else rounded
    result: ac.u64 = rounded if small else original
    canonical: ac.u64 = 9221120237041090560 if wide64 else 2143289344
    result = canonical if nan else result
    supported: ac.u1 = rounding <= 6
    return FloatWordResponse(
        result=result if supported else 0,
        valid=request_valid & supported & ~invalid,
        invalid=invalid & supported,
        overflow=0,
        underflow=0,
        inexact=small & integral.inexact & supported,
        zero=((result & ~sign_mask) == 0) & supported,
        divide_by_zero=0,
    )


@ac.module
def FloatRoundIntegral(  # noqa: N802
    value: ac.u64,
    wide64: ac.u1,
    rounding: ac.u3,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return round_integral(value, wide64, rounding, request_valid)
