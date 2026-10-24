"""RNE binary32 to E2M1/E1M2 nibble, with an explicit overflow endpoint.

kind0 E2M1 has values0,.5,1,1.5,2,3,4,6; kind1 E1M2 has quarter steps0..7/4.
The endpoint is a policy input: consumers can select the exact code required
by their owner. NaN maps to zero, signaling NaN sets NV; infinity sets OF|NX.
"""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.packed4_formats import level, midpoint


def encode_packed4(
    value: ac.u32,
    kind: ac.u1,
    overflow_endpoint: ac.u3,
    request_valid: ac.u1,
) -> FloatWordResponse:
    sign: ac.u1 = value[31:32]
    absolute: ac.u32 = value & 2147483647
    exponent: ac.u8 = value[23:31]
    fraction: ac.u23 = value[:23]
    nan: ac.u1 = (exponent == 255) & (fraction != 0)
    invalid: ac.u1 = nan & ~fraction[22:23]
    maximum: ac.u32 = level(7, kind)
    overflow: ac.u1 = (absolute > maximum) & ~nan
    selected: ac.u3 = 0
    for edge in range(7):
        index: ac.u3 = edge
        candidate: ac.u3 = edge + 1
        boundary: ac.u32 = midpoint(index, kind)
        odd: ac.u1 = (edge & 1) != 0
        take: ac.u1 = (absolute > boundary) | ((absolute == boundary) & odd)
        selected = candidate if take else selected
    selected = overflow_endpoint if overflow else selected
    encoded: ac.u64 = selected
    sign64: ac.u64 = sign
    encoded = encoded | (sign64 << 3)
    encoded = 0 if nan else encoded
    chosen: ac.u32 = level(selected, kind)
    inexact: ac.u1 = (absolute != chosen) & ~nan
    tiny: ac.u1 = (absolute < 1048576000) if kind else (chosen < 1065353216)
    return FloatWordResponse(
        result=encoded,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=overflow,
        underflow=tiny & inexact & ~overflow,
        inexact=inexact | overflow,
        zero=(encoded & 7) == 0,
        divide_by_zero=0,
    )


@ac.module
def Packed4Encode(  # noqa: N802
    value: ac.u32,
    kind: ac.u1,
    overflow_endpoint: ac.u3,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return encode_packed4(value, kind, overflow_endpoint, request_valid)
