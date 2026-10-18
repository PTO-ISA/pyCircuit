"""ResBench modular_add_cipher: authored from the problem and port declaration. Uses the example fixed key 5 explicitly offered in the problem."""

import pycircuit as ac


def compute(data_in: ac.u8) -> ac.u8:
    return data_in + 5


@ac.module
def Top(data_in: ac.u8) -> ac.u8:
    return compute(data_in)
