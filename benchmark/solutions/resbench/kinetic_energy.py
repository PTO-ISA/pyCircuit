"""ResBench kinetic_energy: authored from the problem and port declaration."""

import pycircuit as ac


def compute(mass: ac.u8, velocity: ac.u8) -> ac.u16:
    m: ac.u24 = mass
    v: ac.u24 = velocity
    full = (m * v * v) // 2
    return full[:16]


@ac.module
def Top(mass: ac.u8, velocity: ac.u8) -> ac.u16:
    return compute(mass, velocity)
