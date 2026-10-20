"""Integer-only binary32 to E4M3FN/E5M2 and HiF8 encoding.

FP8 uses RNE. HiF8 additionally exposes RHB (ties toward positive infinity).
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.float_formats import unpack_float, pack_float
from functional_float.lowp_formats import hif8_rank, lower_rank


def encode_lowp(
    value: ac.u32, kind: ac.u2, rounding: ac.u1, saturating: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:
    value64: ac.u64 = value
    source = unpack_float(value64, 0)
    magnitude: ac.bits[128] = source.significand
    precision: ac.u7 = 2 if kind == 1 else 3
    bias: ac.u13 = 15 if kind == 1 else 7
    maximum: ac.u13 = 31 if kind == 1 else 16
    packed = pack_float(
        source.sign, magnitude, source.lsb_bias, precision, bias, maximum, 7
    )
    sign8: ac.u8 = source.sign
    sign64: ac.u64 = source.sign
    payload: ac.u64 = packed.result & 127
    clamp_e4: ac.u1 = packed.overflow | (payload == 127)
    e4: ac.u64 = (sign64 << 7) | 126 if clamp_e4 else packed.result
    e4 = (sign64 << 7) | 126 if source.inf else e4
    e4 = 127 if source.nan else e4
    e5: ac.u64 = (sign64 << 7) | 124 if source.inf else packed.result
    e5 = 126 if source.nan else e5
    result: ac.u64 = e5 if kind == 1 else e4
    finite: ac.u1 = ~(source.nan | source.inf)
    discarded_large: ac.u1 = value[:20] != 0
    e4_inexact: ac.u1 = (packed.inexact & ~packed.overflow) | (
        packed.overflow & discarded_large
    )
    overflow: ac.u1 = packed.overflow & finite & (kind == 1)
    underflow: ac.u1 = packed.underflow & finite
    inexact: ac.u1 = (packed.inexact if kind == 1 else e4_inexact) & finite
    # Relevant finite binary32 values are represented exactly in Q64. Values
    # below half the minimum HiF8 subnormal always choose zero.
    shift13: ac.u13 = source.lsb_bias - 4032
    shift: ac.u8 = 127 if shift13 > 127 else shift13[:8]
    shift = 0 if source.lsb_bias < 4032 else shift
    target: ac.bits[128] = magnitude << shift
    raw_magnitude: ac.u31 = value[:31]
    tiny: ac.u1 = raw_magnitude < 872415232
    target = 0 if tiny else target
    rank: ac.u8 = lower_rank(value[23:31], value[:23])
    lower = hif8_rank(rank)
    upper_rank: ac.u8 = rank if rank == 126 else rank + 1
    upper = hif8_rank(upper_rank)
    down: ac.bits[128] = target - lower.magnitude
    up: ac.bits[128] = upper.magnitude - target
    tie_up: ac.u1 = lower.code[0:1] if rounding == 0 else ~source.sign
    choose_up: ac.u1 = (up < down) | ((up == down) & tie_up)
    code: ac.u8 = upper.code if choose_up else lower.code
    chosen: ac.bits[128] = upper.magnitude if choose_up else lower.magnitude
    hif: ac.u8 = code | (sign8 << 7)
    hif = 0 if code == 0 else hif
    hif_overflow: ac.u1 = (raw_magnitude > 1191182336) & ~source.nan
    maximum_code: ac.u8 = 110 if saturating else 111
    hif = maximum_code | (sign8 << 7) if hif_overflow else hif
    hif = 128 if source.nan else hif
    hif_inexact: ac.u1 = ((chosen != target) | (tiny & (raw_magnitude != 0))) & finite
    hif_inexact = hif_inexact | hif_overflow
    if kind == 2:
        result = hif
        overflow = hif_overflow
        inexact = hif_inexact
        underflow = (raw_magnitude < 939524096) & hif_inexact & finite
    supported: ac.u1 = (kind != 3) & ((kind == 2) | (rounding == 0))
    invalid: ac.u1 = source.snan & supported
    zero: ac.u1 = (result == 0) if kind == 2 else ((result & 127) == 0)
    return FloatWordResponse(
        result=result if supported else 0,
        valid=request_valid & supported & ~invalid,
        invalid=invalid,
        overflow=overflow & supported,
        underflow=underflow & supported,
        inexact=inexact & supported,
        zero=zero & supported,
        divide_by_zero=0,
    )


@ac.module
def LowpEncode(  # noqa: N802
    value: ac.u32,
    kind: ac.u2,
    rounding: ac.u1,
    saturating: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return encode_lowp(value, kind, rounding, saturating, request_valid)
