"""Three directly composed byte incrementer instances."""

import pycircuit as ac


@ac.struct
class IncrementResult:
    y: ac.u8


@ac.module
def Incrementer(x: ac.u8) -> IncrementResult:  # noqa: N802
    return IncrementResult(y=x + 1)


@ac.module
def HierModules(x: ac.u8) -> IncrementResult:  # noqa: N802
    first = Incrementer(x)
    second = Incrementer(first.y)
    third = Incrementer(second.y)
    return third
