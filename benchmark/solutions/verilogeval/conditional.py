import pycircuit as ac


@ac.module
def Top(a: ac.u8, b: ac.u8, c: ac.u8, d: ac.u8) -> ac.u8:  # noqa: N802
    ab = a if a < b else b
    cd = c if c < d else d
    return ab if ab < cd else cd
