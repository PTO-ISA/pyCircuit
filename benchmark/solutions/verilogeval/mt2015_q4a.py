import pycircuit as ac


@ac.module
def Top(x: ac.u1, y: ac.u1) -> ac.u1:  # noqa: N802
    return (x ^ y) & x
