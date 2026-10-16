"""Byte arithmetic selection followed by four modular increments."""

from pycircuit import module, struct, u2, u8


@struct
class JitResult:
    result: u8


def calculate(a: u8, b: u8, op: u2) -> u8:
    result: u8 = 0
    if op == 0:
        result = a + b
    elif op == 1:
        result = a - b
    elif op == 2:
        result = a ^ b
    else:
        result = a & b
    for _step in range(4):
        result = result + 1
    return result


@module
def JitControlFlow(a: u8, b: u8, op: u2) -> JitResult:  # noqa: N802
    return JitResult(result=calculate(a, b, op))
