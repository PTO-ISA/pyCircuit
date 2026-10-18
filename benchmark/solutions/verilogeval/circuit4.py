import pycircuit as ac


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1, d: ac.u1) -> ac.u1:  # noqa: N802
    return b | c
