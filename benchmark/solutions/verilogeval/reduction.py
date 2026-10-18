import pycircuit as ac


@ac.module
def Top(in_: ac.u8) -> ac.u1:  # noqa: N802
    return ac.popcount(in_)[:1]
