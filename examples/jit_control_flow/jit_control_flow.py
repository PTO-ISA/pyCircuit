"""Byte arithmetic selection followed by four modular increments."""

import pycircuit as ac


@ac.struct
class JitResult:
    result: ac.u8


@ac.module
def JitControlFlow(a: ac.u8, b: ac.u8, op: ac.u2) -> JitResult:  # noqa: N802
    sum_mod = a + b
    difference_mod = a - b
    xor_value = a ^ b
    and_value = a & b
    bitwise_value = xor_value if op == 2 else and_value
    other_value = difference_mod if op == 1 else bitwise_value
    selected = sum_mod if op == 0 else other_value

    round_1 = selected + 1
    round_2 = round_1 + 1
    round_3 = round_2 + 1
    result = round_3 + 1
    return JitResult(result=result)
