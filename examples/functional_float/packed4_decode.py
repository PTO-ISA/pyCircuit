"""Exact E2M1/E1M2 nibble to binary32, including both signed zeros."""

import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse
from functional_float.packed4_formats import level


def decode_packed4(
    code: ac.u4,
    kind: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    magnitude: ac.u64 = level(code[:3], kind)
    sign: ac.u64 = code[3:4]
    result: ac.u64 = magnitude | (sign << 31)
    return FloatWordResponse(
        result=result,
        valid=request_valid,
        invalid=0,
        overflow=0,
        underflow=0,
        inexact=0,
        zero=code[:3] == 0,
        divide_by_zero=0,
    )


@ac.module
def Packed4Decode(  # noqa: N802
    code: ac.u4,
    kind: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return decode_packed4(code, kind, request_valid)
