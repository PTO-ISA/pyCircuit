"""ResBench carbon_footprint: authored from the problem and port declaration. Computes the weighted sum exactly in hundredths and floors once at the integral output."""

import pycircuit as ac


def compute(energy: ac.u8, distance: ac.u8, waste: ac.u8) -> ac.u16:
    e: ac.u16 = energy
    d: ac.u16 = distance
    w: ac.u16 = waste
    return (85 * e + 20 * d + 10 * w) // 100


@ac.module
def Top(energy: ac.u8, distance: ac.u8, waste: ac.u8) -> ac.u16:
    return compute(energy, distance, waste)
