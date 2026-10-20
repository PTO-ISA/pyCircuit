"""Separately rounded floor-modulo sequence used by the current SSM consumer.

This is a reusable numerical policy: divide-RNE, floor the floating quotient,
product-RNE, then subtract-RNE. It preserves the consumer's zero-divisor result
of positive zero. It deliberately differs from ASL's truncating remainder and
from an exact-real modulo evaluated with only one final rounding.
"""
import pycircuit as ac
from functional_float.fp32_divide import divide_bits
from functional_float.fp32_product import mul_finite
from functional_float.fp32_sum import add_finite
from functional_float.fp32_types import FP32Response


def floor_bits(value: ac.u32) -> ac.u32:
    exponent: ac.u8 = value[23:31]
    shift: ac.u8 = 150 - exponent
    one: ac.u32 = 1
    lower_mask: ac.u32 = (one << shift) - 1
    truncated: ac.u32 = value & ~lower_mask
    discarded: ac.u1 = (value & lower_mask) != 0
    increment: ac.u32 = one << shift
    rounded: ac.u32 = truncated + increment if value[31:32] & discarded else truncated
    small: ac.u32 = 3212836864 if value[31:32] else 0
    small = value if value[:31] == 0 else small
    result: ac.u32 = small if exponent < 127 else rounded
    return value if exponent >= 150 else result


def floor_modulo(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1) -> FP32Response:
    quotient = divide_bits(lhs, rhs, request_valid)
    integral: ac.u32 = floor_bits(quotient.result)
    product = mul_finite(integral, rhs, request_valid)
    result = add_finite(lhs, product.result ^ 2147483648, request_valid)
    zero_divisor: ac.u1 = rhs[:31] == 0
    invalid: ac.u1 = quotient.invalid | product.invalid | result.invalid
    overflow: ac.u1 = quotient.overflow | product.overflow | result.overflow
    underflow: ac.u1 = quotient.underflow | product.underflow | result.underflow
    inexact: ac.u1 = quotient.inexact | product.inexact | result.inexact
    payload: ac.u32 = 0 if zero_divisor else result.result
    invalid = 0 if zero_divisor else invalid
    return FP32Response(result=payload, valid=request_valid & ~invalid, invalid=invalid,
        overflow=overflow & ~zero_divisor, underflow=underflow & ~zero_divisor,
        inexact=inexact & ~zero_divisor, zero=payload[:31] == 0)


@ac.module
def FP32FloorModulo(  # noqa: N802
    lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1,
) -> FP32Response:
    return floor_modulo(lhs, rhs, request_valid)
