"""binary32 maximumNumber bit graph, including NaNs and signed-zero ties.

This preserves the accepted numeric/IEEE-invalid distinction: a signaling
NaN reports NV but still returns the deterministic selected payload. It is
not a protocol rejection or a host floating-point comparison.
"""

import pycircuit as ac
from functional_float.fp32_types import FP32Response


def maximum(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1, minimum: ac.u1) -> FP32Response:
    a_nan: ac.u1 = (lhs[23:31] == 255) & (lhs[:23] != 0)
    b_nan: ac.u1 = (rhs[23:31] == 255) & (rhs[:23] != 0)
    invalid: ac.u1 = (a_nan & ~lhs[22:23]) | (b_nan & ~rhs[22:23])
    signs_differ: ac.u1 = lhs[31:32] != rhs[31:32]
    greater: ac.u1 = lhs[:31] > rhs[:31]
    reversed_order: ac.u1 = lhs[:31] < rhs[:31]
    lhs_positive: ac.u1 = ~lhs[31:32]
    greater = reversed_order if lhs[31:32] else greater
    greater = lhs_positive if signs_differ else greater
    greater = ~greater if minimum else greater
    result: ac.u32 = lhs if greater else rhs
    both_zero: ac.u1 = (lhs[:31] == 0) & (rhs[:31] == 0)
    zero_result: ac.u32 = (lhs | rhs) if minimum else (lhs & rhs)
    result = zero_result if both_zero else result
    result = rhs if a_nan else result
    result = lhs if b_nan else result
    result = 2143289344 if a_nan & b_nan else result
    zero: ac.u1 = result[:31] == 0
    return FP32Response(
        result=result, valid=request_valid, invalid=request_valid & invalid,
        overflow=0, underflow=0, inexact=0, zero=zero,
    )


@ac.module
def FP32Max(  # noqa: N802
    lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1, minimum: ac.u1
) -> FP32Response:
    return maximum(lhs, rhs, request_valid, minimum)
