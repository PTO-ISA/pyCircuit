from typing import Annotated

from pycircuit import dff, module


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    state = dff(T=Annotated[int, range(1 << 8)])
    state = a
    return {"result": a}
