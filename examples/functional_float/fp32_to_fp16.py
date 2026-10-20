"""Bit-level binary32 -> binary16 conversion source unit.

The conversion is authored for the public pyCircuit 412a180 source-unit
frontend and uses only fixed-width integer bits, static slices, conditional
selection and the public sticky-right-shift helper.  It performs
round-to-nearest, ties-to-even (RNE) without host floating point or legacy
arithmetic.  The result carrier includes the same IEEE-style effect flags used
by the other functional source units.
"""

import pycircuit as ac

from functional_float.fp16_types import FP16Response


@ac.struct
class Shifted32:
    data: ac.u32
    sticky: ac.u1


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


def convert(value: ac.u32, request_valid: ac.u1) -> FP16Response:
    sign: ac.u1 = value[31:32]
    exp32: ac.u8 = value[23:31]
    frac: ac.u23 = value[:23]
    frac_zero: ac.u1 = frac == 0
    is_zero: ac.u1 = (exp32 == 0) & frac_zero
    is_subnormal32: ac.u1 = (exp32 == 0) & (~frac_zero)
    is_inf: ac.u1 = (exp32 == 255) & frac_zero
    is_nan: ac.u1 = (exp32 == 255) & (~frac_zero)
    is_snan: ac.u1 = is_nan & (~frac[22:23])

    one: ac.u1 = 1
    mant24: ac.u24 = ac.concat(one, frac)
    mant32: ac.u32 = mant24

    # Binary32 normals below exp32=113 are binary16 subnormals.  The shift
    # keeps 11 result bits (one implicit bit plus ten fraction bits) and the
    # guard/round/sticky bits below them.  Shifts beyond 31 are clamped: every
    # such value is far below the binary16 half-ulp and rounds to zero.
    small: ac.u1 = (exp32 != 0) & (exp32 < 113)
    small_shift: ac.u8 = 113 - exp32
    shift_raw: ac.u8 = (small_shift + 13) if small else 13
    shift: ac.u8 = 31 if shift_raw > 31 else shift_raw

    shifted = right_barrel32_sticky(mant32, shift)
    truncated: ac.u11 = shifted.data[0:11]
    guard_shift: ac.u8 = shift - 1
    round_shift: ac.u8 = shift - 2
    guard: ac.u1 = right_barrel32_sticky(mant32, guard_shift).data[0:1]
    rounded_bits = right_barrel32_sticky(mant32, round_shift)
    round_bit: ac.u1 = rounded_bits.data[0:1]
    sticky: ac.u1 = rounded_bits.sticky

    increment: ac.u12 = 1 if (guard & (round_bit | sticky | truncated[0:1])) else 0
    truncated12: ac.u12 = truncated
    rounded: ac.u12 = truncated12 + increment
    rounded_ge_400: ac.u1 = rounded >= 0x400
    rounded_ge_800: ac.u1 = rounded >= 0x800

    carry8: ac.u8 = rounded_ge_800
    exp16_normal_raw: ac.u8 = (exp32 - 112) + carry8
    exp16_small: ac.u8 = 1 if rounded_ge_400 else 0
    exp16_raw: ac.u8 = exp16_small if small else exp16_normal_raw
    overflow_finite: ac.u1 = exp16_raw >= 31
    finite_overflow: ac.u1 = overflow_finite & (~(is_zero | is_subnormal32))
    exp16: ac.u5 = 31 if overflow_finite else exp16_raw[:5]
    frac10: ac.u10 = rounded[0:10]

    finite_inexact_raw: ac.u1 = guard | round_bit | sticky
    finite_inexact: ac.u1 = finite_inexact_raw | overflow_finite
    finite_underflow: ac.u1 = small & (~rounded_ge_400) & finite_inexact_raw
    rounded_zero: ac.u1 = rounded == 0

    sign16: ac.u16 = sign
    exp16_16: ac.u16 = exp16
    frac16: ac.u16 = frac10
    sign_field: ac.u16 = sign16 << 15
    finite_packed: ac.u16 = (
        sign_field | (exp16_16 << 10) | frac16
    )
    inf_packed: ac.u16 = sign_field | 0x7C00
    qnan_packed: ac.u16 = sign_field | 0x7E00
    zero_packed: ac.u16 = sign_field

    result: ac.u16 = (
        qnan_packed
        if is_nan
        else (
            inf_packed
            if (is_inf | finite_overflow)
            else (
                zero_packed
                if (is_zero | is_subnormal32 | rounded_zero)
                else finite_packed
            )
        )
    )
    inexact: ac.u1 = (
        0
        if (is_zero | is_nan | is_inf)
        else (1 if is_subnormal32 else finite_inexact)
    )
    underflow: ac.u1 = (
        0
        if (~(small | is_subnormal32))
        else (1 if is_subnormal32 else finite_underflow)
    )
    overflow: ac.u1 = finite_overflow & (~(is_nan | is_inf))
    zero: ac.u1 = is_zero | is_subnormal32 | (
        rounded_zero & (~(is_nan | is_inf | is_zero))
    )
    invalid: ac.u1 = request_valid & is_snan
    valid: ac.u1 = request_valid & (~invalid)
    return FP16Response(
        result=result,
        valid=valid,
        invalid=invalid,
        overflow=overflow,
        underflow=underflow,
        inexact=inexact,
        zero=zero,
    )


@ac.module
def FP32ToFP16(value: ac.u32, request_valid: ac.u1) -> FP16Response:  # noqa: N802
    return convert(value, request_valid)
