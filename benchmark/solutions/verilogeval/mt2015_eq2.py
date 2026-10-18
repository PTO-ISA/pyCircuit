import pycircuit as ac


@ac.module
def Top(A: ac.u2, B: ac.u2) -> ac.u1:  # noqa: N802, N803
    return A == B
