import pycircuit as ac


@ac.module
def FunctionA(x: ac.u1, y: ac.u1) -> ac.u1:  # noqa: N802
    return (x ^ y) & x


@ac.module
def FunctionB(x: ac.u1, y: ac.u1) -> ac.u1:  # noqa: N802
    return ~(x ^ y)


@ac.module
def Top(x: ac.u1, y: ac.u1) -> ac.u1:  # noqa: N802
    first = FunctionA(x, y) | FunctionB(x, y)
    second = FunctionA(x, y) & FunctionB(x, y)
    return first ^ second
