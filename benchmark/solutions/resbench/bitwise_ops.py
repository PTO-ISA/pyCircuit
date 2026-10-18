"""ResBench bitwise_ops: authored from the problem and port declaration."""

import pycircuit as ac


@ac.struct
class Output:
    and_out: ac.u8
    or_out: ac.u8
    xor_out: ac.u8


def compute(a: ac.u8, b: ac.u8) -> Output:
    return Output(and_out=a & b, or_out=a | b, xor_out=a ^ b)


@ac.module
def Top(a: ac.u8, b: ac.u8) -> Output:
    return compute(a, b)
