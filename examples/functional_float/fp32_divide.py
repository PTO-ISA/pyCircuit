"""Binary32 RNE quotient using normalized integer division and sticky remainder.

The quotient retains eight extra significand bits before shared RNE packing.
Special paths use a nonzero denominator and return explicit IEEE payloads.
"""
import pycircuit as ac
from functional_float.fp32_product import unpack_fp32, round_pack


@ac.struct
class FP32QuotientResponse:
    result: ac.u32
    valid: ac.u1
    invalid: ac.u1
    overflow: ac.u1
    underflow: ac.u1
    inexact: ac.u1
    zero: ac.u1
    divide_by_zero: ac.u1


def divide_bits(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1) -> FP32QuotientResponse:
    a = unpack_fp32(lhs)
    b = unpack_fp32(rhs)
    az: ac.u5 = ac.count_leading_zeros(a.mant)
    bz: ac.u5 = ac.count_leading_zeros(b.mant)
    am: ac.u64 = a.mant
    bm: ac.u64 = b.mant
    am = am << az
    bm = bm << bz
    denominator: ac.u64 = 1 if b.zero else bm
    numerator: ac.u64 = am << 32
    quotient: ac.u64 = numerator // denominator
    remainder: ac.u64 = numerator % denominator
    sticky: ac.u64 = 1 if remainder != 0 else 0
    magnitude: ac.u64 = quotient | sticky
    ae: ac.u11 = a.e
    be: ac.u11 = b.e
    ashift: ac.u11 = az
    bshift: ac.u11 = bz
    sign: ac.u1 = a.sign ^ b.sign
    final = round_pack(sign, magnitude, ae - be + bshift - ashift + 480)
    invalid: ac.u1 = a.snan | b.snan | (a.inf & b.inf) | (a.zero & b.zero)
    any_nan: ac.u1 = a.nan | b.nan | invalid
    special: ac.u1 = any_nan | a.inf | b.inf | a.zero | b.zero
    sign32: ac.u32 = sign
    result: ac.u32 = final.result
    result = sign32 << 31 if (a.zero | b.inf) else result
    result = (sign32 << 31) | 2139095040 if (a.inf | b.zero) else result
    result = 2143289344 if any_nan else result
    return FP32QuotientResponse(
        result=result, valid=request_valid & ~invalid, invalid=invalid,
        overflow=final.overflow & ~special, underflow=final.underflow & ~special,
        inexact=final.inexact & ~special, zero=result[:31] == 0,
        divide_by_zero=(~a.nan) & (~b.nan) & (~a.inf) & (~a.zero) & b.zero,
    )


@ac.module
def FP32Divide(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1) -> FP32QuotientResponse:  # noqa: N802
    return divide_bits(lhs, rhs, request_valid)
