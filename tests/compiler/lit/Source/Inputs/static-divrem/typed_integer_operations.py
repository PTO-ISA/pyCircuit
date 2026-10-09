"""Historical typed-integer result packing through the current source route."""

import pycircuit as ac


@ac.struct
class Result:
    value: ac.u64


@ac.rule
def calculate(value) -> Result:
    divisor: ac.u64 = value & 255
    quotient = value // divisor
    remainder = value % divisor
    quotient_low: ac.u64 = quotient[:16]
    remainder_low: ac.u64 = remainder[:8]
    constant_quotient = value // 8
    constant_quotient_low: ac.u64 = constant_quotient[:8]
    constant_remainder = value % 8
    narrow = value[:3]
    widened: ac.u64 = narrow
    prefix: ac.u5 = 31 if narrow[2:3] else 0
    sign_extended_low: ac.u64 = ac.concat(prefix, narrow)
    constant: ac.u64 = 17
    zero: ac.u64 = 0
    return Result(
        value=(
            zero
            | quotient_low
            | (remainder_low << 16)
            | (widened << 24)
            | (sign_extended_low << 32)
            | (constant << 40)
            | (constant_quotient_low << 48)
            | (constant_remainder << 56)
        )
    )


@ac.module
def TypedIntegerOperations(value: ac.u64) -> Result:  # noqa: N802 - hardware module
    return calculate(value)
