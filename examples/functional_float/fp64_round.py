"""Unpack and RNE packing of exact binary64 integer magnitudes."""
import pycircuit as ac
from functional_float.fp64_types import FloatWordResponse, FP64Parts


def unpack_fp64(value: ac.u64) -> FP64Parts:
    exponent: ac.u11 = value[52:63]
    fraction: ac.u52 = value[:52]
    one: ac.u1 = 1
    zero_bit: ac.u1 = 0
    normal: ac.u53 = ac.concat(one, fraction)
    subnormal: ac.u53 = ac.concat(zero_bit, fraction)
    nan: ac.u1 = (exponent == 2047) & (fraction != 0)
    return FP64Parts(sign=value[63:64], exponent=1 if exponent == 0 else exponent,
        mantissa=subnormal if exponent == 0 else normal,
        zero=(exponent == 0) & (fraction == 0),
        inf=(exponent == 2047) & (fraction == 0), nan=nan,
        snan=nan & ~fraction[51:52])


def round_fp64(sign: ac.u1, magnitude: ac.bits[128], lsb_bias: ac.u13) -> FloatWordResponse:
    # Exact value = magnitude * 2**(lsb_bias - 4096).
    zeros: ac.u8 = ac.count_leading_zeros(magnitude)
    leading: ac.u8 = 127 - zeros
    leading = 0 if magnitude == 0 else leading
    leading13: ac.u13 = leading
    exponent_plus: ac.u13 = lsb_bias + leading13
    normal: ac.u1 = exponent_plus >= 3074
    normal_shift: ac.u13 = leading13 + 4044
    subnormal_shift: ac.u13 = leading13 + 7118 - exponent_plus
    shift_bias: ac.u13 = normal_shift if normal else subnormal_shift
    right: ac.u1 = shift_bias >= 4096
    right13: ac.u13 = shift_bias - 4096 if right else 0
    left13: ac.u13 = 4096 - shift_bias if ~right else 0
    right_amount: ac.u8 = 255 if right13 > 255 else right13[:8]
    left_amount: ac.u7 = 127 if left13 > 127 else left13[:7]
    truncated: ac.bits[128] = magnitude >> right_amount if right else magnitude << left_amount
    guard13: ac.u13 = right13 - 1
    guard_amount: ac.u8 = 255 if guard13 > 255 else guard13[:8]
    guard: ac.u1 = (magnitude >> guard_amount)[0:1]
    guard = guard if right & (right13 >= 1) else 0
    one: ac.bits[128] = 1
    lower_mask: ac.bits[128] = (one << guard_amount) - 1
    sticky: ac.u1 = (magnitude & lower_mask) != 0
    discarded: ac.u1 = (guard | sticky) if right & (right13 >= 1) else 0
    increment: ac.bits[128] = 1 if guard & (sticky | truncated[0:1]) else 0
    rounded: ac.bits[128] = truncated + increment
    normal_carry: ac.u1 = rounded[53:54]
    subnormal_carry: ac.u1 = rounded[52:53]
    normal_exponent: ac.u13 = exponent_plus - 3073
    normal_exponent = normal_exponent + 1 if normal_carry else normal_exponent
    subnormal_exponent: ac.u13 = 1 if subnormal_carry else 0
    exponent: ac.u13 = normal_exponent if normal else subnormal_exponent
    fraction: ac.u52 = 0 if (normal_carry if normal else subnormal_carry) else rounded[:52]
    overflow: ac.u1 = exponent > 2046
    packed_exponent: ac.u11 = 2047 if overflow else exponent[:11]
    fraction = 0 if overflow else fraction
    packed: ac.u64 = ac.concat(sign, packed_exponent, fraction)
    sign64: ac.u64 = sign
    result: ac.u64 = sign64 << 63 if (magnitude == 0) | (rounded == 0) else packed
    return FloatWordResponse(result=result, valid=1, invalid=0,
        overflow=overflow, underflow=(exponent == 0) & discarded,
        inexact=discarded | overflow, zero=result[:63] == 0, divide_by_zero=0)
