"""Explicit one-round fused multiply-add for four raw floating formats."""

import pycircuit as ac
from functional_float.float_fused import fused_formats
from functional_float.fp64_types import FloatWordResponse


@ac.module
def FloatFusedFormat(  # noqa: N802
    lhs: ac.u64,
    rhs: ac.u64,
    addend: ac.u64,
    format_code: ac.u2,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return fused_formats(lhs, rhs, addend, format_code, request_valid)
