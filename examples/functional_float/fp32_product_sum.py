"""Separately rounded binary32 product followed by sum.

IEEE exceptions from both operations are retained. An invalid product is an
arithmetic result, not a rejected request: Sum still consumes its NaN payload.
"""

import pycircuit as ac

from functional_float.fp32_product import FP32Product
from functional_float.fp32_sum import FP32Sum
from functional_float.fp32_types import FP32Response


@ac.module
def FP32ProductSum(  # noqa: N802
    lhs: ac.u32, rhs: ac.u32, acc: ac.u32, request_valid: ac.u1
) -> FP32Response:
    product = FP32Product(lhs, rhs, request_valid)
    total = FP32Sum(product.result, acc, request_valid)
    return FP32Response(
        result=total.result,
        valid=product.valid & total.valid,
        invalid=product.invalid | total.invalid,
        overflow=product.overflow | total.overflow,
        underflow=product.underflow | total.underflow,
        inexact=product.inexact | total.inexact,
        zero=total.zero,
    )
