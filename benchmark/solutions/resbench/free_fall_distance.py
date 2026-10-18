"""ResBench free_fall_distance: authored from the problem and port declaration."""

import pycircuit as ac


def compute(t_in: ac.u8) -> ac.u16:
    t: ac.u16 = t_in
    return 5 * t * t


@ac.module
def Top(t_in: ac.u8) -> ac.u16:
    return compute(t_in)
