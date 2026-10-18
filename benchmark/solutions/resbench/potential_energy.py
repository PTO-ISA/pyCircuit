"""ResBench potential_energy: authored from the problem and port declaration."""

import pycircuit as ac


def compute(mass: ac.u8, height: ac.u8) -> ac.u16:
    m: ac.u16 = mass
    h: ac.u16 = height
    return 10 * m * h


@ac.module
def Top(mass: ac.u8, height: ac.u8) -> ac.u16:
    return compute(mass, height)
