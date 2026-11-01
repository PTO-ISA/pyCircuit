import pycircuit as ac


def bump(value: ac.u8) -> ac.u8:
    return value + 3


def calculate(value: ac.u8) -> tuple[ac.u8, ac.u16]:
    result = bump(value)
    return result, ac.u16(result << ac.u8(1))
