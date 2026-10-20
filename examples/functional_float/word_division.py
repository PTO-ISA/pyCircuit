"""Reusable signed/unsigned quotient with an explicit zero-denominator policy.

The general WordArithmetic provider retains its zero quotient default. This
composition can instead request all-one bits, as the current SSM scalar
consumer does. Both policies are generated; neither invokes host division.
"""
import pycircuit as ac
from functional_float.word_arithmetic import WordResult, word_arithmetic


def word_division(lhs: ac.u64, rhs: ac.u64, is_signed: ac.u1, word32: ac.u1,
                  zero_ones: ac.u1, request_valid: ac.u1) -> WordResult:
    operation: ac.u4 = 11 if is_signed else 3
    quotient = word_arithmetic(lhs, rhs, operation, word32, request_valid)
    denominator: ac.u64 = rhs & 4294967295 if word32 else rhs
    result: ac.u64 = 18446744073709551615 if zero_ones & (denominator == 0) else quotient.result
    return WordResult(result=result, valid=quotient.valid)


@ac.module
def WordDivision(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, is_signed: ac.u1, word32: ac.u1,
    zero_ones: ac.u1, request_valid: ac.u1,
) -> WordResult:
    return word_division(lhs, rhs, is_signed, word32, zero_ones, request_valid)
