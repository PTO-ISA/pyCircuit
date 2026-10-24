"""One-round binary32/64 multiply-add for explicitly fused consumers.

This provider is separate from ProductSum, whose two roundings are unchanged.
"""

import pycircuit as ac
from functional_float.float_formats import (
    exponent_max,
    fraction_bits,
    round_format,
    sign_position,
    unpack_float,
)
from functional_float.fp64_types import FloatWordResponse


def shift_jam(value: ac.bits[128], amount: ac.u13) -> ac.bits[128]:
    shift: ac.u8 = 255 if amount > 255 else amount[:8]
    one: ac.bits[128] = 1
    mask: ac.bits[128] = (one << shift) - 1
    sticky: ac.bits[128] = 1 if (value & mask) != 0 else 0
    return (value >> shift) | sticky


def fused_formats(
    lhs: ac.u64, rhs: ac.u64, addend: ac.u64, format_code: ac.u2, request_valid: ac.u1
) -> FloatWordResponse:
    precision: ac.u7 = fraction_bits(format_code)
    a = unpack_float(lhs, format_code)
    b = unpack_float(rhs, format_code)
    c = unpack_float(addend, format_code)
    am: ac.bits[128] = a.significand
    bm: ac.bits[128] = b.significand
    cm: ac.bits[128] = c.significand
    product_pad: ac.u7 = 125 - (precision << 1)
    addend_pad: ac.u7 = 126 - precision
    product_pad13: ac.u13 = product_pad
    addend_pad13: ac.u13 = addend_pad
    product_lsb: ac.u13 = a.lsb_bias + b.lsb_bias - 4096 - product_pad13
    addend_lsb: ac.u13 = c.lsb_bias - addend_pad13
    product: ac.bits[128] = (am * bm) << product_pad
    cm = cm << addend_pad
    product_zeros: ac.u8 = ac.count_leading_zeros(product)
    addend_zeros: ac.u8 = ac.count_leading_zeros(cm)
    product_normalize: ac.u7 = product_zeros[:7] - 1
    addend_normalize: ac.u7 = addend_zeros[:7] - 1
    product_normalize = 0 if product == 0 else product_normalize
    addend_normalize = 0 if cm == 0 else addend_normalize
    product_normalize13: ac.u13 = product_normalize
    addend_normalize13: ac.u13 = addend_normalize
    product = product << product_normalize
    cm = cm << addend_normalize
    product_lsb = product_lsb - product_normalize13
    addend_lsb = addend_lsb - addend_normalize13
    high: ac.u13 = product_lsb if product_lsb >= addend_lsb else addend_lsb
    high = addend_lsb if product == 0 else high
    high = product_lsb if cm == 0 else high
    p = shift_jam(product, high - product_lsb)
    q = shift_jam(cm, high - addend_lsb)
    product_sign: ac.u1 = a.sign ^ b.sign
    same_sign: ac.u1 = product_sign == c.sign
    p_larger: ac.u1 = p >= q
    difference: ac.bits[128] = p - q if p_larger else q - p
    magnitude: ac.bits[128] = p + q if same_sign else difference
    sign: ac.u1 = product_sign if p_larger else c.sign
    sign = product_sign if same_sign else sign
    sign = 0 if (magnitude == 0) & ~same_sign else sign
    rounded = round_format(sign, magnitude, high, format_code)
    product_inf: ac.u1 = (a.inf | b.inf) & ~(a.nan | b.nan)
    bad_product: ac.u1 = (a.inf & b.zero) | (b.inf & a.zero)
    invalid: ac.u1 = a.snan | b.snan | c.snan | bad_product
    invalid = invalid | (product_inf & c.inf & (product_sign != c.sign))
    any_nan: ac.u1 = a.nan | b.nan | c.nan | invalid
    any_inf: ac.u1 = product_inf | c.inf
    maximum: ac.u64 = exponent_max(format_code)
    infinity_sign: ac.u64 = product_sign if product_inf else c.sign
    position: ac.u6 = sign_position(format_code)
    infinity: ac.u64 = (infinity_sign << position) | (maximum << precision)
    one: ac.u64 = 1
    quiet_position: ac.u7 = precision - 1
    qnan: ac.u64 = (maximum << precision) | (one << quiet_position)
    result: ac.u64 = infinity if any_inf else rounded.result
    result = qnan if any_nan else result
    finite: ac.u1 = ~(any_nan | any_inf)
    return FloatWordResponse(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=rounded.overflow & finite,
        underflow=rounded.underflow & finite,
        inexact=rounded.inexact & finite,
        zero=rounded.zero & finite,
        divide_by_zero=0,
    )


def fused_bits(
    lhs: ac.u64, rhs: ac.u64, addend: ac.u64, wide64: ac.u1, request_valid: ac.u1
) -> FloatWordResponse:
    format_code: ac.u2 = 1 if wide64 else 0
    return fused_formats(lhs, rhs, addend, format_code, request_valid)


@ac.module
def FloatFused(  # noqa: N802
    lhs: ac.u64,
    rhs: ac.u64,
    addend: ac.u64,
    wide64: ac.u1,
    request_valid: ac.u1,
) -> FloatWordResponse:
    return fused_bits(lhs, rhs, addend, wide64, request_valid)
