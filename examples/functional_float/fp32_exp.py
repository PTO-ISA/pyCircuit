"""Binary32 exponential using separate RNE range reduction and Horner steps.

The final scale is split into two exact powers of two. The first product stays
normal and exact; the second performs the single final RNE rounding, including
subnormals. This reuses generated Product instead of expanding another rounder.
"""

import pycircuit as ac
from functional_float.fp32_types import FP32Response
from functional_float.fp32_product import FP32Product, right_barrel64_sticky
from functional_float.fp32_sum import FP32Sum


@ac.struct
class RangeInteger:
    result: ac.u32
    exponent: ac.u11


@ac.struct
class ScalePowers:
    residual: ac.u32
    primary: ac.u32


def bounded_value(value: ac.u32) -> ac.u32:
    return 0 if value[:31] > 1121058816 else value


def range_integer(scaled: ac.u32) -> RangeInteger:
    one: ac.u1 = 1
    mant: ac.u64 = ac.concat(one, scaled[:23])
    e: ac.u9 = scaled[23:31]
    shift9: ac.u9 = 150 - e
    shift: ac.u7 = 127 if shift9 > 127 else shift9[:7]
    whole = right_barrel64_sticky(mant, shift)
    guard_shift: ac.u7 = shift - 1
    tail = right_barrel64_sticky(mant, guard_shift)
    increment: ac.u64 = tail.data[0:1] & (tail.sticky | whole.data[0:1])
    magnitude: ac.u64 = whole.data + increment
    magnitude11: ac.u11 = magnitude[:11]
    negative_n: ac.u1 = scaled[31:32]
    n: ac.u11 = (0 - magnitude11) if negative_n else magnitude11
    # The bounded reduction gives |n| <= 152, so conversion is exact.
    magnitude32: ac.u32 = magnitude[:32]
    zeros: ac.u6 = ac.count_leading_zeros(magnitude32)
    leading: ac.u6 = 31 - zeros
    leading = 0 if magnitude32 == 0 else leading
    leading8: ac.u8 = leading
    float_e: ac.u8 = leading8 + 127
    left_shift: ac.u5 = 23 - leading[:5]
    normalized: ac.u32 = magnitude32 << left_shift
    word: ac.u32 = ac.concat(negative_n, float_e, normalized[:23])
    sign32: ac.u32 = negative_n
    word = sign32 << 31 if magnitude32 == 0 else word
    return RangeInteger(result=word, exponent=n)


def scale_powers(exponent: ac.u11) -> ScalePowers:
    biased: ac.u11 = exponent + 512
    base: ac.u11 = 386 if biased < 386 else biased
    base = 639 if biased > 639 else base
    primary_e: ac.u32 = base - 385
    residual_e: ac.u32 = biased - base + 127
    return ScalePowers(residual=residual_e << 23, primary=primary_e << 23)


def finish_exp(
    value: ac.u32, final: FP32Response, input_inexact: ac.u1, request_valid: ac.u1
) -> FP32Response:
    nan: ac.u1 = (value[23:31] == 255) & (value[:23] != 0)
    inf: ac.u1 = value[:31] == 2139095040
    invalid: ac.u1 = nan & ~value[22:23]
    outside: ac.u1 = value[:31] > 1121058816
    finite: ac.u1 = ~(nan | inf)
    result: ac.u32 = final.result
    outside_result: ac.u32 = 0 if value[31:32] else 2139095040
    result = outside_result if outside else result
    result = 2143289344 if nan else result
    return FP32Response(
        result=result,
        valid=request_valid & ~invalid,
        invalid=invalid,
        overflow=finite & (final.overflow | (outside & ~value[31:32])),
        underflow=finite & (final.underflow | (outside & value[31:32])),
        inexact=finite & (input_inexact | final.inexact | outside),
        zero=result[:31] == 0,
    )


@ac.module
def FP32Exp(value: ac.u32, request_valid: ac.u1) -> FP32Response:  # noqa: N802
    scaled = FP32Product(bounded_value(value), 1069066811, request_valid)
    integer = range_integer(scaled.result)
    reduced_product = FP32Product(integer.result, 1060205080, request_valid)
    reduced = FP32Sum(
        bounded_value(value), reduced_product.result ^ 2147483648, request_valid
    )
    # Horner coefficients: 1/720, 1/120, 1/24, 1/6, 1/2, 1, 1.
    p0 = FP32Product(985008993, reduced.result, request_valid)
    s0 = FP32Sum(p0.result, 1007192201, request_valid)
    p1 = FP32Product(s0.result, reduced.result, request_valid)
    s1 = FP32Sum(p1.result, 1026206379, request_valid)
    p2 = FP32Product(s1.result, reduced.result, request_valid)
    s2 = FP32Sum(p2.result, 1042983595, request_valid)
    p3 = FP32Product(s2.result, reduced.result, request_valid)
    s3 = FP32Sum(p3.result, 1056964608, request_valid)
    p4 = FP32Product(s3.result, reduced.result, request_valid)
    s4 = FP32Sum(p4.result, 1065353216, request_valid)
    p5 = FP32Product(s4.result, reduced.result, request_valid)
    s5 = FP32Sum(p5.result, 1065353216, request_valid)
    powers = scale_powers(integer.exponent)
    intermediate = FP32Product(s5.result, powers.residual, request_valid)
    final = FP32Product(intermediate.result, powers.primary, request_valid)
    inexact = (
        scaled.inexact
        | reduced_product.inexact
        | reduced.inexact
        | p0.inexact
        | s0.inexact
        | p1.inexact
        | s1.inexact
        | p2.inexact
        | s2.inexact
        | p3.inexact
        | s3.inexact
        | p4.inexact
        | s4.inexact
        | p5.inexact
        | s5.inexact
        | intermediate.inexact
    )
    return finish_exp(value, final, inexact, request_valid)
