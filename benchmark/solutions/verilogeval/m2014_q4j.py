import pycircuit as ac


def add(x: ac.u4, y: ac.u4) -> ac.u5:
    left: ac.u5 = x
    right: ac.u5 = y
    return left + right


@ac.module
def Top(x: ac.u4, y: ac.u4) -> ac.u5:  # noqa: N802
    return add(x, y)
