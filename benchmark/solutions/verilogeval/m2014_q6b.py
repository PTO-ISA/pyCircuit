import pycircuit as ac


@ac.module
def Top(y: ac.u3, w: ac.u1) -> ac.u1:  # noqa: N802
    return (y == 1) or (y == 5) or (w and ((y == 2) or (y == 4)))
