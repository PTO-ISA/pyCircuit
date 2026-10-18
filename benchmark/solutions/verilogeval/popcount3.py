import pycircuit as ac


@ac.module
def Top(in_: ac.u3) -> ac.u2:  # noqa: N802
    return ac.popcount(in_)
