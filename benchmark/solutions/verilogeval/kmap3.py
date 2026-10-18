import pycircuit as ac


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1, d: ac.u1) -> ac.u1:  # noqa: N802
    value = ac.concat(a, b, c, d)
    out = (
        (value == 2)
        | (value == 3)
        | (value == 8)
        | (value == 10)
        | (value == 11)
        | (value == 12)
        | (value == 14)
        | (value == 15)
    )
    return out
