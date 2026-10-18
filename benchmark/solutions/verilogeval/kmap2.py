import pycircuit as ac


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1, d: ac.u1) -> ac.u1:  # noqa: N802
    value = ac.concat(a, b, c, d)
    out = (
        (value == 0)
        | (value == 1)
        | (value == 2)
        | (value == 4)
        | (value == 6)
        | (value == 7)
        | (value == 8)
        | (value == 9)
        | (value == 11)
        | (value == 15)
    )
    return out
