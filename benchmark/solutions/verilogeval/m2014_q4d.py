import pycircuit as ac


@ac.rule
def accumulate(parity: ac.u1, incoming: ac.u1) -> None:
    parity = parity ^ incoming


@ac.module
def Top(incoming: ac.u1) -> ac.u1:  # noqa: N802
    parity: ac.u1 = 0
    accumulate(parity, incoming)
    return parity
