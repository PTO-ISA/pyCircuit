import pycircuit as ac


@ac.rule
def advance(count: ac.u4, enabled: ac.u1) -> None:
    if enabled:
        count = 0 if count == 9 else count + 1


@ac.module
def Top(slowena: ac.u1) -> ac.u4:  # noqa: N802
    count: ac.u4 = 0
    advance(count, slowena)
    return count
