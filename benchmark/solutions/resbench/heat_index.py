"""ResBench heat_index: authored from the problem and port declaration."""

import pycircuit as ac


def compute(
    temperature: ac.u8,
    humidity: ac.u8,
    C1: ac.u16,
    C2: ac.u16,
    C3: ac.u16,
    C4: ac.u16,
    C5: ac.u16,
    C6: ac.u16,
    C7: ac.u16,
    C8: ac.u16,
    C9: ac.u16,
) -> ac.u16:
    t: ac.u16 = temperature
    r: ac.u16 = humidity
    return (
        C1
        + C2 * t
        + C3 * r
        + C4 * t * r
        + C5 * t * t
        + C6 * r * r
        + C7 * t * t * r
        + C8 * t * r * r
        + C9 * t * t * r * r
    )


@ac.module
def Top(
    temperature: ac.u8,
    humidity: ac.u8,
    C1: ac.u16,
    C2: ac.u16,
    C3: ac.u16,
    C4: ac.u16,
    C5: ac.u16,
    C6: ac.u16,
    C7: ac.u16,
    C8: ac.u16,
    C9: ac.u16,
) -> ac.u16:
    return compute(temperature, humidity, C1, C2, C3, C4, C5, C6, C7, C8, C9)
