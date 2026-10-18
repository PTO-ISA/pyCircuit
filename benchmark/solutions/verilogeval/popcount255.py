import pycircuit as ac


@ac.module
def Top(in_: ac.bits[255]) -> ac.u8:  # noqa: N802
    return ac.popcount(in_)
