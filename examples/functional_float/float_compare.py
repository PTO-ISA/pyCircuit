"""Ordered binary32/binary64 comparisons with explicit IEEE invalid status."""
import pycircuit as ac


@ac.struct
class FloatCompareResponse:
    result: ac.u1
    invalid: ac.u1
    valid: ac.u1


def compare_float(lhs: ac.u64, rhs: ac.u64, wide64: ac.u1,
                  operation: ac.u2, signaling: ac.u1, request_valid: ac.u1) -> FloatCompareResponse:
    a: ac.u64 = lhs if wide64 else lhs & 4294967295
    b: ac.u64 = rhs if wide64 else rhs & 4294967295
    a_sign: ac.u1 = a[63:64] if wide64 else a[31:32]
    b_sign: ac.u1 = b[63:64] if wide64 else b[31:32]
    a_nan64: ac.u1 = (a[52:63] == 2047) & (a[:52] != 0)
    b_nan64: ac.u1 = (b[52:63] == 2047) & (b[:52] != 0)
    a_nan32: ac.u1 = (a[23:31] == 255) & (a[:23] != 0)
    b_nan32: ac.u1 = (b[23:31] == 255) & (b[:23] != 0)
    a_nan: ac.u1 = a_nan64 if wide64 else a_nan32
    b_nan: ac.u1 = b_nan64 if wide64 else b_nan32
    a_quiet: ac.u1 = a[51:52] if wide64 else a[22:23]
    b_quiet: ac.u1 = b[51:52] if wide64 else b[22:23]
    a_mag: ac.u64 = a & 9223372036854775807 if wide64 else a & 2147483647
    b_mag: ac.u64 = b & 9223372036854775807 if wide64 else b & 2147483647
    both_zero: ac.u1 = (a_mag == 0) & (b_mag == 0)
    equal: ac.u1 = (a == b) | both_zero
    positive_less: ac.u1 = a_mag < b_mag
    negative_less: ac.u1 = a_mag > b_mag
    less: ac.u1 = negative_less if a_sign else positive_less
    less = a_sign if a_sign != b_sign else less
    less = 0 if both_zero else less
    result: ac.u1 = equal
    result = ~equal if operation == 1 else result
    result = less if operation == 2 else result
    result = ~less if operation == 3 else result
    any_nan: ac.u1 = a_nan | b_nan
    any_signalling: ac.u1 = (a_nan & ~a_quiet) | (b_nan & ~b_quiet)
    invalid: ac.u1 = (signaling & any_nan) | any_signalling
    return FloatCompareResponse(result=result & ~any_nan, invalid=invalid,
                                valid=request_valid)


@ac.module
def FloatCompare(  # noqa: N802
lhs: ac.u64, rhs: ac.u64, wide64: ac.u1,
                 operation: ac.u2, signaling: ac.u1, request_valid: ac.u1) -> FloatCompareResponse:  # noqa: N802
    return compare_float(lhs, rhs, wide64, operation, signaling, request_valid)
