import pycircuit as ac


@ac.module
def Top(x3: ac.u1, x2: ac.u1, x1: ac.u1) -> ac.u1:  # noqa: N802
    return (~x3 & x2) | (x3 & x1)
