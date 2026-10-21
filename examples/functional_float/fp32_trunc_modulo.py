"""Exact binary32 remainder with an RTZ quotient, without a rounded divide.

Finite nonzero operands use normalized integer modular reduction. The exact
remainder is representable in binary32, including subnormals; no intermediate
floating rounding or FMA is used. Exact cancellation encodes positive zero;
a zero dividend retains its sign. Invalid inputs return a canonical NaN.
The separately published FP32FloorModulo remains a distinct staged policy.
"""
import pycircuit as ac
from functional_float.fp32_product import unpack_fp32
from functional_float.fp32_types import FP32Response


def trunc_modulo(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1) -> FP32Response:
    a = unpack_fp32(lhs)
    b = unpack_fp32(rhs)
    az: ac.u5 = ac.count_leading_zeros(a.mant)
    bz: ac.u5 = ac.count_leading_zeros(b.mant)
    am: ac.u25 = a.mant
    bm: ac.u25 = b.mant
    am = am << az
    bm = bm << bz
    bm = 1 if b.zero else bm
    ae: ac.u10 = a.e
    be: ac.u10 = b.e
    ashift: ac.u10 = az
    bshift: ac.u10 = bz
    ae = ae + 32 - ashift
    be = be + 32 - bshift
    distance: ac.u10 = ae - be
    denominator: ac.u64 = bm
    numerator: ac.u64 = am
    residual: ac.u64 = numerator % denominator
    factor: ac.u64 = 2
    # Binary modular exponentiation covers the complete exponent gap 0..276.
    # All products are below 2**48; zero divisors use a harmless denominator.
    for bit in range(9):
        reduced: ac.u64 = (residual * factor) % denominator
        residual = reduced if (distance >> bit)[0:1] else residual
        factor = (factor * factor) % denominator
    remainder: ac.u25 = residual[:25]
    leading: ac.u5 = ac.count_leading_zeros(remainder)
    normalize: ac.u5 = leading - 1
    norm: ac.u32 = remainder
    norm = norm << normalize
    nshift: ac.u10 = normalize
    encoded_exp: ac.u10 = be + 224 - nshift
    exp32: ac.u32 = encoded_exp - 256
    subshift: ac.u10 = 257 - encoded_exp
    payload: ac.u32 = ((exp32 << 23) | (norm & 8388607)) if encoded_exp > 256 else norm >> subshift
    sign: ac.u32 = a.sign
    payload = (sign << 31) | payload
    payload = 0 if remainder == 0 else payload
    payload = lhs if lhs[:31] < rhs[:31] else payload
    payload = lhs if b.inf | a.zero else payload
    invalid: ac.u1 = a.snan | b.snan | a.inf | b.zero
    payload = 2143289344 if a.nan | b.nan | invalid else payload
    return FP32Response(result=payload, valid=request_valid & ~invalid,
        invalid=invalid, overflow=0, underflow=0, inexact=0,
        zero=payload[:31] == 0)


@ac.module
def FP32TruncModulo(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1) -> FP32Response:  # noqa: N802
    return trunc_modulo(lhs, rhs, request_valid)
