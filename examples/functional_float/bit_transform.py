"""Wrapping bit fields, counts, selection and comparisons over 64-bit words."""
import pycircuit as ac
from functional_float.word_arithmetic import WordResult


def bit_transform(lhs: ac.u64, rhs: ac.u64, alternate: ac.u64,
                  width: ac.u7, offset: ac.u6, last: ac.u6,
                  operation: ac.u4, request_valid: ac.u1) -> WordResult:
    one: ac.u64 = 1
    offset7: ac.u7 = offset
    inverse: ac.u7 = 64 - offset7
    mask: ac.u64 = (one << width) - 1
    rotated: ac.u64 = (lhs >> offset) | (lhs << inverse)
    field: ac.u64 = rotated & mask
    sign_position: ac.u7 = width - 1
    sign: ac.u1 = (field >> sign_position)[0:1]
    signed_field: ac.u64 = field | ~mask if sign else field
    distance: ac.u6 = last - offset
    distance7: ac.u7 = distance
    span: ac.u7 = distance7 + 1
    insertion_mask: ac.u64 = (one << span) - 1
    positioned_mask: ac.u64 = (insertion_mask << offset) | (insertion_mask >> inverse)
    insertion: ac.u64 = rhs & insertion_mask
    positioned: ac.u64 = (insertion << offset) | (insertion >> inverse)
    inserted: ac.u64 = (lhs & ~positioned_mask) | (positioned & positioned_mask)
    zero_count: ac.u7 = ac.count_leading_zeros(field)
    leading: ac.u7 = zero_count - (64 - width)
    isolated: ac.u64 = field & (0 - field)
    isolated_zeros: ac.u7 = ac.count_leading_zeros(isolated)
    trailing: ac.u7 = 63 - isolated_zeros
    trailing = width if field == 0 else trailing
    counted: ac.u64 = field - ((field >> 1) & 6148914691236517205)
    counted = (counted & 3689348814741910323) + ((counted >> 2) & 3689348814741910323)
    counted = (counted + (counted >> 4)) & 1085102592571150095
    population: ac.u64 = (counted * 72340172838076673) >> 56
    leading64: ac.u64 = leading
    trailing64: ac.u64 = trailing
    unsigned_less: ac.u1 = lhs < rhs
    signed_less: ac.u1 = lhs[63:64] if lhs[63:64] != rhs[63:64] else unsigned_less
    equal: ac.u1 = lhs == rhs
    and_set: ac.u1 = (lhs & rhs) != 0
    or_set: ac.u1 = (lhs | rhs) != 0
    value: ac.u64 = field
    value = signed_field if operation == 1 else value
    value = inserted if operation == 2 else value
    value = leading64 if operation == 3 else value
    value = trailing64 if operation == 4 else value
    value = population if operation == 5 else value
    selected: ac.u64 = rhs if lhs != 0 else alternate
    value = selected if operation == 6 else value
    equal_value: ac.u64 = 1 if equal else 0
    value = equal_value if operation == 8 else value
    not_equal_value: ac.u64 = 0 if equal else 1
    value = not_equal_value if operation == 9 else value
    signed_less_value: ac.u64 = 1 if signed_less else 0
    value = signed_less_value if operation == 10 else value
    unsigned_less_value: ac.u64 = 1 if unsigned_less else 0
    value = unsigned_less_value if operation == 11 else value
    and_value: ac.u64 = 1 if and_set else 0
    value = and_value if operation == 12 else value
    or_value: ac.u64 = 1 if or_set else 0
    value = or_value if operation == 13 else value
    signed_ge_value: ac.u64 = 0 if signed_less else 1
    value = signed_ge_value if operation == 14 else value
    unsigned_ge_value: ac.u64 = 0 if unsigned_less else 1
    value = unsigned_ge_value if operation == 15 else value
    return WordResult(result=value, valid=request_valid & (width >= 1) & (width <= 64) & (operation != 7))


@ac.module
def BitTransform(  # noqa: N802
    lhs: ac.u64, rhs: ac.u64, alternate: ac.u64, width: ac.u7,
    offset: ac.u6, last: ac.u6, operation: ac.u4, request_valid: ac.u1,
) -> WordResult:
    return bit_transform(lhs, rhs, alternate, width, offset, last, operation, request_valid)
