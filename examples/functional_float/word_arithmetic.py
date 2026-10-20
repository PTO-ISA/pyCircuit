"""Reusable 32/64-bit integer arithmetic, signed division and shifts.

Selectors describe arithmetic functions, independently of any instruction set.
Results wrap to the selected width; word32 results are sign extended. A zero
quotient denominator returns zero, while zero-denominator remainder returns
the dividend. Unknown selectors reject the request explicitly.
"""
import pycircuit as ac


@ac.struct
class WordResult:
    result: ac.u64
    valid: ac.u1


def word_arithmetic(lhs: ac.u64, rhs: ac.u64, operation: ac.u4,
                    word32: ac.u1, request_valid: ac.u1) -> WordResult:
    a: ac.u64 = lhs & 4294967295 if word32 else lhs
    b: ac.u64 = rhs & 4294967295 if word32 else rhs
    a_signed: ac.u64 = a | 18446744069414584320 if word32 & a[31:32] else a
    b_signed: ac.u64 = b | 18446744069414584320 if word32 & b[31:32] else b
    a_negative: ac.u1 = a_signed[63:64]
    b_negative: ac.u1 = b_signed[63:64]
    a_magnitude: ac.u64 = 0 - a_signed if a_negative else a_signed
    b_magnitude: ac.u64 = 0 - b_signed if b_negative else b_signed
    denominator: ac.u64 = 1 if b == 0 else b
    magnitude_denominator: ac.u64 = 1 if b_magnitude == 0 else b_magnitude
    unsigned_quotient: ac.u64 = a // denominator
    unsigned_quotient = 0 if b == 0 else unsigned_quotient
    unsigned_remainder: ac.u64 = a % denominator
    unsigned_remainder = a if b == 0 else unsigned_remainder
    magnitude_quotient: ac.u64 = a_magnitude // magnitude_denominator
    signed_quotient: ac.u64 = 0 - magnitude_quotient if a_negative ^ b_negative else magnitude_quotient
    signed_quotient = 0 if b == 0 else signed_quotient
    signed_remainder: ac.u64 = a_signed - signed_quotient * b_signed
    amount: ac.u6 = rhs[:6]
    narrow_amount: ac.u6 = rhs[:5]
    amount = narrow_amount if word32 else amount
    amount7: ac.u7 = amount
    fill_amount: ac.u7 = 64 - amount7
    ones: ac.u64 = 18446744073709551615
    sign_fill: ac.u64 = ones << fill_amount if a_negative else 0
    arithmetic_shift: ac.u64 = (a_signed >> amount) | sign_fill
    value: ac.u64 = a + b
    value = a - b if operation == 1 else value
    value = a * b if operation == 2 else value
    value = unsigned_quotient if operation == 3 else value
    value = unsigned_remainder if operation == 4 else value
    value = a & b if operation == 5 else value
    value = a | b if operation == 6 else value
    value = a ^ b if operation == 7 else value
    value = a << amount if operation == 8 else value
    value = a >> amount if operation == 9 else value
    value = arithmetic_shift if operation == 10 else value
    value = signed_quotient if operation == 11 else value
    value = signed_remainder if operation == 12 else value
    low: ac.u64 = value & 4294967295
    word: ac.u64 = low | 18446744069414584320 if value[31:32] else low
    return WordResult(result=word if word32 else value,
                      valid=request_valid & (operation <= 12))


@ac.module
def WordArithmetic(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, operation: ac.u4,
    word32: ac.u1, request_valid: ac.u1,
) -> WordResult:
    return word_arithmetic(lhs, rhs, operation, word32, request_valid)
