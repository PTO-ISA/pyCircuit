"""ResBench wavelength: authored from the problem and port declaration."""

import pycircuit as ac


def compute(frequency: ac.u8) -> ac.u8:
    f: ac.u16 = frequency
    speed: ac.u16 = 300
    quotient = speed // f
    return quotient[:8] if frequency != 0 else 0


@ac.module
def Top(frequency: ac.u8) -> ac.u8:
    return compute(frequency)
