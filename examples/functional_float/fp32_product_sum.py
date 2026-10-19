"""Separately-rounded FP32 product-sum source unit.

The multiply and add arithmetic live in independent source units.  This unit
composes their published interfaces with direct nested calls, so each frontend
evaluation stays under the 412a180 per-source budget.  The multiply is rounded
to binary32 before the add, matching the separate MUL-round -> ADD-round
contract rather than FMA.

412a180 rejects an imported Struct construction, a local binding of an
imported typed-struct module result ("persistent initializer must be a static
complete value"), and a direct module call under runtime control ("direct
module calls under runtime control are unsupported").  The direct nested-call
form is the minimal admitted rewrite.  It instantiates Product twice: once for
the rounded operand and once for the invalid gate.  Product invalid
short-circuits the Sum request; `valid` is gated by both IEEE invalid results,
while invalid/overflow/underflow/inexact/zero are combined across Product and
Sum.  A protocol request rejection remains distinct from an IEEE exception.
"""

import pycircuit as ac

from functional_float.fp32_product import FP32Product
from functional_float.fp32_sum import FP32Sum
from functional_float.fp32_types import FP32Response


@ac.module
def FP32ProductSum(  # noqa: N802
    lhs: ac.u32, rhs: ac.u32, acc: ac.u32, request_valid: ac.u1
) -> FP32Response:
    return FP32Sum(
        FP32Product(lhs, rhs, request_valid).result,
        acc,
        request_valid & (~FP32Product(lhs, rhs, request_valid).invalid),
    )
