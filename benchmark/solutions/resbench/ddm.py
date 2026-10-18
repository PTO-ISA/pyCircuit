"""ResBench ddm: authored from the problem and port declaration. Returns zero when required return is no greater than growth; rates use the specified Q0.8 format."""

import pycircuit as ac


def compute(
    expected_dividend: ac.u16, required_rate_of_return: ac.u8, growth_rate: ac.u8
) -> ac.u32:
    dividend: ac.u32 = expected_dividend
    required: ac.u32 = required_rate_of_return
    growth: ac.u32 = growth_rate
    result = (dividend << 8) // (required - growth)
    return result if required > growth else 0


@ac.module
def Top(
    expected_dividend: ac.u16, required_rate_of_return: ac.u8, growth_rate: ac.u8
) -> ac.u32:
    return compute(expected_dividend, required_rate_of_return, growth_rate)
