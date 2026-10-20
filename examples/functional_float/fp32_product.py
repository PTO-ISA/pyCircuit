"""Bit-level separately-rounded binary32 Product source unit.

This source is authored for the public pyCircuit 412a180 source-unit frontend
(``import pycircuit as ac``).  It intentionally uses only fixed-width unsigned
bits, static slices, static shifts, conditional selections and the public
leading-zero helper.  There is no host floating point, FMA, legacy generated
binary or truncating finite-normal shortcut in this file.

Rounding is round-to-nearest, ties-to-even (RNE).  The module returns a raw
binary32 word plus explicit IEEE-style flags.  ``valid`` means the request was
accepted and the operation has a defined IEEE result; it is false only for an
invalid operation.  Quiet NaNs propagate as the canonical word 0x7fc00000 and
do not set ``invalid``; signalling NaNs and 0*Inf are invalid and also return
that canonical NaN.
"""

import pycircuit as ac

from functional_float.fp32_types import FP32Response


@ac.struct
class Unpacked:
    sign: ac.u1
    e: ac.u8
    mant: ac.u24
    zero: ac.u1
    inf: ac.u1
    nan: ac.u1
    snan: ac.u1


@ac.struct
class Shifted64:
    data: ac.u64
    sticky: ac.u1


@ac.struct
class Shifted32:
    data: ac.u32
    sticky: ac.u1


@ac.struct
class Rounded:
    result: ac.u32
    overflow: ac.u1
    underflow: ac.u1
    inexact: ac.u1
    zero: ac.u1


def unpack_fp32(value: ac.u32) -> Unpacked:
    exp: ac.u8 = value[23:31]
    frac: ac.u23 = value[:23]
    sign: ac.u1 = value[31:32]
    zero: ac.u1 = (exp == 0) & (frac == 0)
    inf: ac.u1 = (exp == 255) & (frac == 0)
    nan: ac.u1 = (exp == 255) & (frac != 0)
    snan: ac.u1 = nan & (~frac[22:23])
    one_bit: ac.u1 = 1
    zero_bit: ac.u1 = 0
    normal_mant: ac.u24 = ac.concat(one_bit, frac)
    subnormal_mant: ac.u24 = ac.concat(zero_bit, frac)
    e: ac.u8 = exp if exp != 0 else 1
    mant: ac.u24 = subnormal_mant if exp == 0 else normal_mant
    return Unpacked(
        sign=sign, e=e, mant=mant, zero=zero, inf=inf, nan=nan, snan=snan
    )


def right_barrel64_sticky(value: ac.u64, amount: ac.u7) -> Shifted64:
    shifted: ac.u64 = value
    sticky: ac.u1 = 0

    discarded0: ac.u1 = shifted[0:1]
    shifted = (shifted >> 1) if amount[0:1] else shifted
    sticky = (sticky | discarded0) if amount[0:1] else sticky

    discarded1: ac.u1 = shifted[:2] != 0
    shifted = (shifted >> 2) if amount[1:2] else shifted
    sticky = (sticky | discarded1) if amount[1:2] else sticky

    discarded2: ac.u1 = shifted[:4] != 0
    shifted = (shifted >> 4) if amount[2:3] else shifted
    sticky = (sticky | discarded2) if amount[2:3] else sticky

    discarded3: ac.u1 = shifted[:8] != 0
    shifted = (shifted >> 8) if amount[3:4] else shifted
    sticky = (sticky | discarded3) if amount[3:4] else sticky

    discarded4: ac.u1 = shifted[:16] != 0
    shifted = (shifted >> 16) if amount[4:5] else shifted
    sticky = (sticky | discarded4) if amount[4:5] else sticky

    discarded5: ac.u1 = shifted[:32] != 0
    shifted = (shifted >> 32) if amount[5:6] else shifted
    sticky = (sticky | discarded5) if amount[5:6] else sticky

    discarded6: ac.u1 = shifted != 0
    shifted = 0 if amount[6:7] else shifted
    sticky = (sticky | discarded6) if amount[6:7] else sticky
    return Shifted64(data=shifted, sticky=sticky)


def left_barrel64(value: ac.u64, amount: ac.u5) -> ac.u64:
    shifted: ac.u64 = value
    shifted = (shifted << 1) if amount[0:1] else shifted
    shifted = (shifted << 2) if amount[1:2] else shifted
    shifted = (shifted << 4) if amount[2:3] else shifted
    shifted = (shifted << 8) if amount[3:4] else shifted
    shifted = (shifted << 16) if amount[4:5] else shifted
    return shifted


def right_barrel32_sticky(value: ac.u32, amount: ac.u8) -> Shifted32:
    shifted: ac.u32 = value
    sticky: ac.u1 = 0

    discarded0: ac.u1 = shifted[0:1]
    shifted = (shifted >> 1) if amount[0:1] else shifted
    sticky = (sticky | discarded0) if amount[0:1] else sticky

    discarded1: ac.u1 = shifted[:2] != 0
    shifted = (shifted >> 2) if amount[1:2] else shifted
    sticky = (sticky | discarded1) if amount[1:2] else sticky

    discarded2: ac.u1 = shifted[:4] != 0
    shifted = (shifted >> 4) if amount[2:3] else shifted
    sticky = (sticky | discarded2) if amount[2:3] else sticky

    discarded3: ac.u1 = shifted[:8] != 0
    shifted = (shifted >> 8) if amount[3:4] else shifted
    sticky = (sticky | discarded3) if amount[3:4] else sticky

    discarded4: ac.u1 = shifted[:16] != 0
    shifted = (shifted >> 16) if amount[4:5] else shifted
    sticky = (sticky | discarded4) if amount[4:5] else sticky

    discarded5: ac.u1 = shifted != 0
    shifted = 0 if amount[5:6] else shifted
    sticky = (sticky | discarded5) if amount[5:6] else sticky

    discarded6: ac.u1 = shifted != 0
    shifted = 0 if amount[6:7] else shifted
    sticky = (sticky | discarded6) if amount[6:7] else sticky

    discarded7: ac.u1 = shifted != 0
    shifted = 0 if amount[7:8] else shifted
    sticky = (sticky | discarded7) if amount[7:8] else sticky
    return Shifted32(data=shifted, sticky=sticky)


def round_pack(sign: ac.u1, magnitude: ac.u64, lsb_bias: ac.u11) -> Rounded:
    """Round an unsigned magnitude to binary32 with RNE and pack it.

    The exact value is ``magnitude * 2**(lsb_bias - 512)``.  The helper derives
    the leading-bit exponent, selects normal or subnormal precision, shifts
    with guard/round/sticky, and applies ties-to-even.  No host arithmetic is
    used.
    """
    leading_zeros: ac.u7 = ac.count_leading_zeros(magnitude)
    leading: ac.u7 = 63 - leading_zeros
    leading = 0 if magnitude == 0 else leading
    leading11: ac.u11 = leading
    e_plus: ac.u11 = lsb_bias + leading11
    normal: ac.u1 = e_plus >= 386

    shift_normal11: ac.u11 = leading11 + 489
    shift_sub11: ac.u11 = (leading11 + 875) - e_plus
    shift_bias: ac.u11 = shift_normal11 if normal else shift_sub11
    is_right: ac.u1 = shift_bias >= 512
    right_amount11: ac.u11 = (shift_bias - 512) if is_right else 0
    left_amount11: ac.u11 = (512 - shift_bias) if (~is_right) else 0
    right_amount: ac.u7 = 127 if right_amount11 > 127 else right_amount11[:7]
    left_amount: ac.u5 = 31 if left_amount11 > 31 else left_amount11[:5]

    right_shifted = right_barrel64_sticky(magnitude, right_amount)
    left_shifted: ac.u64 = left_barrel64(magnitude, left_amount)
    truncated: ac.u64 = right_shifted.data if is_right else left_shifted

    guard_amount11: ac.u11 = right_amount11 - 1
    guard_amount: ac.u7 = 127 if guard_amount11 > 127 else guard_amount11[:7]
    guard_ok: ac.u1 = is_right & (right_amount11 >= 1)
    guard_bit: ac.u1 = right_barrel64_sticky(magnitude, guard_amount).data[0:1]
    guard: ac.u1 = guard_bit if guard_ok else 0

    round_amount11: ac.u11 = right_amount11 - 2
    round_amount: ac.u7 = 127 if round_amount11 > 127 else round_amount11[:7]
    round_ok: ac.u1 = is_right & (right_amount11 >= 2)
    round_bit: ac.u1 = right_barrel64_sticky(magnitude, round_amount).data[0:1]
    round_out: ac.u1 = round_bit if round_ok else 0

    sticky_amount11: ac.u11 = (right_amount11 - 2) if (right_amount11 >= 2) else 0
    sticky_amount: ac.u7 = 127 if sticky_amount11 > 127 else sticky_amount11[:7]
    sticky_bit: ac.u1 = right_barrel64_sticky(magnitude, sticky_amount).sticky
    sticky: ac.u1 = sticky_bit if is_right else 0

    increment_cond: ac.u1 = guard & (round_out | sticky | truncated[0:1])
    increment: ac.u64 = 1 if increment_cond else 0
    rounded: ac.u64 = truncated + increment

    normal_carry: ac.u1 = rounded[24:25]
    subnormal_carry: ac.u1 = rounded[23:24]
    exp_normal11: ac.u11 = e_plus - 385
    exp_normal_carry11: ac.u11 = exp_normal11 + 1
    exp_normal_sel: ac.u11 = exp_normal_carry11 if normal_carry else exp_normal11
    exp_subnormal: ac.u11 = 1 if subnormal_carry else 0
    exp_final: ac.u11 = exp_normal_sel if normal else exp_subnormal

    frac_normal: ac.u23 = 0 if normal_carry else rounded[0:23]
    frac_subnormal: ac.u23 = 0 if subnormal_carry else rounded[0:23]
    frac_final: ac.u23 = frac_normal if normal else frac_subnormal

    overflow: ac.u1 = exp_final > 254
    exp_packed: ac.u8 = 255 if overflow else exp_final[:8]
    frac_packed: ac.u23 = 0 if overflow else frac_final
    packed: ac.u32 = ac.concat(sign, exp_packed, frac_packed)

    zero: ac.u1 = (magnitude == 0) | (rounded == 0)
    sign32: ac.u32 = sign
    signed_zero: ac.u32 = sign32 << 31
    result: ac.u32 = signed_zero if zero else packed
    inexact_raw: ac.u1 = is_right & (guard | round_out | sticky)
    inexact: ac.u1 = inexact_raw | overflow
    underflow: ac.u1 = (~normal) & inexact_raw
    return Rounded(
        result=result,
        overflow=overflow,
        underflow=underflow,
        inexact=inexact,
        zero=zero,
    )


def mul_finite(lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1) -> FP32Response:
    a = unpack_fp32(lhs)
    b = unpack_fp32(rhs)
    # 412a180 bits multiplication keeps the operand width; widen before the
    # multiply so the full 24x24 product survives.
    a_mant64: ac.u64 = a.mant
    b_mant64: ac.u64 = b.mant
    product64: ac.u64 = a_mant64 * b_mant64
    a_e11: ac.u11 = a.e
    b_e11: ac.u11 = b.e
    lsb_bias: ac.u11 = (a_e11 + b_e11) + 212
    sign: ac.u1 = a.sign ^ b.sign
    rounded = round_pack(sign, product64, lsb_bias)

    nan_any: ac.u1 = a.nan | b.nan
    inf_any: ac.u1 = a.inf | b.inf
    inf_zero: ac.u1 = (a.inf & b.zero) | (a.zero & b.inf)
    invalid: ac.u1 = (a.snan | b.snan) | inf_zero
    qnan: ac.u32 = 0x7FC00000
    inf_sign32: ac.u32 = a.sign ^ b.sign
    inf_result: ac.u32 = (inf_sign32 << 31) | 0x7F800000
    special_nan: ac.u1 = nan_any | invalid
    result: ac.u32 = qnan if special_nan else (inf_result if inf_any else rounded.result)
    finite_used: ac.u1 = ~(nan_any | inf_any)
    return FP32Response(
        result=result,
        valid=request_valid & (~invalid),
        invalid=invalid,
        overflow=rounded.overflow & finite_used,
        underflow=rounded.underflow & finite_used,
        inexact=rounded.inexact & finite_used,
        zero=rounded.zero & finite_used,
    )


@ac.module
def FP32Product(  # noqa: N802
    lhs: ac.u32, rhs: ac.u32, request_valid: ac.u1
) -> FP32Response:
    return mul_finite(lhs, rhs, request_valid)
