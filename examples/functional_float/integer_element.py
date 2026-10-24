"""Fixed-width integer value transformations for numeric tile consumers.

Operation selectors are consumer adapter inputs, not PTO instructions. The
model uses only public source capture, fixed-width bits, and arithmetic.
Division by zero is a rejected request, never a fabricated quotient.
"""

import pycircuit as ac


@ac.struct
class IntegerResponse:
    result: ac.u32
    valid: ac.u1


def integer_element(
    lhs: ac.u32,
    rhs: ac.u32,
    operation: ac.u4,
    narrow16: ac.u1,
    descending: ac.u1,
    request_valid: ac.u1,
) -> IntegerResponse:
    a: ac.u32 = lhs & 65535 if narrow16 else lhs
    b: ac.u32 = rhs & 65535 if narrow16 else rhs
    divisor: ac.u32 = b if b != 0 else 1
    value: ac.u32 = a + b
    value = a - b if operation == 1 else value
    value = a * b if operation == 2 else value
    value = a // divisor if operation == 3 else value
    value = a % divisor if operation == 4 else value
    value = a if operation == 5 else value
    sequence: ac.u32 = a - b if descending else a + b
    value = sequence if operation == 6 else value
    result: ac.u32 = value & 65535 if narrow16 else value
    divides: ac.u1 = (operation == 3) | (operation == 4)
    return IntegerResponse(
        result=result,
        valid=request_valid & (operation <= 6) & (~divides | (b != 0)),
    )


@ac.module
def IntegerElement(  # noqa: N802
    lhs: ac.u32,
    rhs: ac.u32,
    operation: ac.u4,
    narrow16: ac.u1,
    descending: ac.u1,
    request_valid: ac.u1,
) -> IntegerResponse:
    return integer_element(lhs, rhs, operation, narrow16, descending, request_valid)
