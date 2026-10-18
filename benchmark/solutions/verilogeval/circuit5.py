import pycircuit as ac


@ac.module
def Top(a: ac.u4, b: ac.u4, c: ac.u4, d: ac.u4, e: ac.u4) -> ac.u4:  # noqa: N802
    q = b if c == 0 else e if c == 1 else a if c == 2 else d if c == 3 else 15
    return q
