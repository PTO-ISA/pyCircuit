"""One-round finite binary64 to E4M3/E5M2/HiF8 within finite output range.

The explicitly bounded domain is finite |x| <= the selected maximum finite
value. Out-of-domain requests reject. HiF8 supports RNE and RHB; FP8 RNE.
Exact Q96 distances retain all binary64 bits at HiF8 rounding boundaries.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.float_formats import unpack_float, pack_float
from functional_float.lowp_formats import lower_rank, hif8_rank


def finite_cast(
    value: ac.u64, kind: ac.u2, rhb: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:
    source = unpack_float(value, 1)
    magnitude: ac.bits[128] = source.significand
    precision: ac.u7 = 2 if kind == 1 else 3
    bias: ac.u13 = 15 if kind == 1 else 7
    maximum: ac.u13 = 31 if kind == 1 else 16
    packed = pack_float(
        source.sign, magnitude, source.lsb_bias, precision, bias, maximum, 7
    )
    absolute: ac.u64 = value & 9223372036854775807
    bound: ac.u64 = 4646588915539509248
    bound = 4678114112931102720 if kind == 1 else bound
    bound = 4674736413210574848 if kind == 2 else bound
    supported: ac.u1 = (kind != 3) & ~source.nan & ~source.inf & (absolute <= bound)
    supported = supported & ((kind == 2) | ~rhb)
    exponent: ac.u11 = value[52:63]
    exp13: ac.u13 = exponent
    proxy13: ac.u13 = exp13 - 896
    proxy: ac.u8 = proxy13[:8]
    tiny: ac.u1 = absolute < 4503599627370496000
    proxy = 0 if tiny else proxy
    rank: ac.u8 = lower_rank(proxy, value[29:52])
    lower = hif8_rank(rank)
    upper_rank: ac.u8 = rank if rank == 126 else rank + 1
    upper = hif8_rank(upper_rank)
    shift13: ac.u13 = source.lsb_bias - 4000
    shift: ac.u8 = shift13[:8]
    shift = 0 if source.lsb_bias < 4000 else shift
    target: ac.bits[128] = magnitude << shift
    target = 0 if tiny else target
    low: ac.bits[128] = lower.magnitude << 32
    high: ac.bits[128] = upper.magnitude << 32
    down: ac.bits[128] = target - low
    up: ac.bits[128] = high - target
    tie_up: ac.u1 = ~source.sign if rhb else lower.code[0:1]
    choose: ac.u1 = (up < down) | ((up == down) & tie_up)
    code: ac.u8 = upper.code if choose else lower.code
    chosen: ac.bits[128] = high if choose else low
    sign: ac.u8 = source.sign
    hif: ac.u8 = code | (sign << 7)
    hif = 0 if code == 0 else hif
    hif_inexact: ac.u1 = (chosen != target) | (tiny & (absolute != 0))
    hif64: ac.u64 = hif
    result: ac.u64 = packed.result
    result = hif64 if kind == 2 else result
    inexact: ac.u1 = hif_inexact if kind == 2 else packed.inexact
    underflow: ac.u1 = (chosen < 2417851639229258349412352) & hif_inexact
    underflow = packed.underflow if kind != 2 else underflow
    zero: ac.u1 = (result == 0) if kind == 2 else ((result & 127) == 0)
    return FloatWordResponse(
        result=result if supported else 0,
        valid=request_valid & supported,
        invalid=0,
        overflow=0,
        underflow=underflow & supported,
        inexact=inexact & supported,
        zero=zero & supported,
        divide_by_zero=0,
    )


@ac.module
def FiniteLowpCast(  # noqa: N802
    value: ac.u64, kind: ac.u2, rhb: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:
    return finite_cast(value, kind, rhb, request_valid)
